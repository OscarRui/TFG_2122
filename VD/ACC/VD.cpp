#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <string.h>

#include <openacc.h>
#include <cublas_v2.h>
#include <cuda_runtime_api.h>
#include <cuda.h>

#include <cusolverDn.h>
#include <cuda_runtime.h>
#include <assert.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define MAXLINE 200
#define MAXCAD 200
#define FPS 5

/*
 * Author: Jorge Sevilla Cedillo
 * Centre: Universidad de Extremadura
 * */
void cleanString(char *cadena, char *out)
{
    int i,j;
    for( i = j = 0; cadena[i] != 0;++i)
    {
        if(isalnum(cadena[i])||cadena[i]=='{'||cadena[i]=='.'||cadena[i]==',')
        {
            out[j]=cadena[i];
            j++;
        }
    }
    for( i = j; out[i] != 0;++i)
        out[j]=0;
}

/*
 * Author: Jorge Sevilla Cedillo
 * Centre: Universidad de Extremadura
 * */
int readHeader1(char* filename, int *lines, int *samples, int *bands, int *dataType,
		char* interleave, int *byteOrder, char* waveUnit)
{
    FILE *fp;
    char line[MAXLINE]="";
    char value [MAXLINE]="";

    if ((fp=fopen(filename,"rt"))!=NULL)
    {
        fseek(fp,0L,SEEK_SET);
        while(fgets(line, MAXLINE, fp)!= NULL)
        {
            //Samples
            if(strstr(line, "samples")!=NULL && samples !=NULL)
            {
                cleanString(strstr(line, "="),value);
                *samples = atoi(value);
            }

            //Lines
            if(strstr(line, "lines")!=NULL && lines !=NULL)
            {
                cleanString(strstr(line, "="),value);
                *lines = atoi(value);
            }

            //Bands
            if(strstr(line, "bands")!=NULL && bands !=NULL)
            {
                cleanString(strstr(line, "="),value);
                *bands = atoi(value);
            }

            //Interleave
            if(strstr(line, "interleave")!=NULL && interleave !=NULL)
            {
                cleanString(strstr(line, "="),value);
                strcpy(interleave,value);
            }

            //Data Type
            if(strstr(line, "data type")!=NULL && dataType !=NULL)
            {
                cleanString(strstr(line, "="),value);
                *dataType = atoi(value);
            }

            //Byte Order
            if(strstr(line, "byte order")!=NULL && byteOrder !=NULL)
            {
                cleanString(strstr(line, "="),value);
                *byteOrder = atoi(value);
            }

            //Wavelength Unit
            if(strstr(line, "wavelength unit")!=NULL && waveUnit !=NULL)
            {
                cleanString(strstr(line, "="),value);
                strcpy(waveUnit,value);
            }

        }
        fclose(fp);
        return 0;
    }
    else
    	return -2; //No file found
}

/*
 * Author: Jorge Sevilla Cedillo
 * Centre: Universidad de Extremadura
 * */
int readHeader2(char* filename, double* wavelength)
{
    FILE *fp;
    char line[MAXLINE]="";
    char value [MAXLINE]="";

    if ((fp=fopen(filename,"rt"))!=NULL)
    {
        fseek(fp,0L,SEEK_SET);
        while(fgets(line, MAXLINE, fp)!= NULL)
        {
            //Wavelength
            if(strstr(line, "wavelength =")!=NULL && wavelength !=NULL)
            {
                char strAll[100000]=" ";
                char *pch;
                int cont = 0;
                do
                {
                    fgets(line, 200, fp);
                    cleanString(line,value);
                    strcat(strAll,value);
                } while(strstr(line, "}")==NULL);

                pch = strtok(strAll,",");

                while (pch != NULL)
                {
                    wavelength[cont]= atof(pch);
                    pch = strtok (NULL, ",");
                    cont++;
                }
            }

		}
		fclose(fp);
		return 0;
	}
	else
		return -2; //No file found
}


/*
 * Author: Jorge Sevilla Cedillo
 * Centre: Universidad de Extremadura
 * */
int loadImage(char* filename, double* image, int lines, int samples, int bands, int dataType, char* interleave)
{

    FILE *fp;
    short int *tipo_short_int;
    float *tipo_float;
    double * tipo_double;
    unsigned int *tipo_uint;
    int i, j, k, op;
    long int lines_samples = lines*samples;


    if ((fp=fopen(filename,"rb"))!=NULL)
    {

        fseek(fp,0L,SEEK_SET);
        tipo_float = (float*)malloc(lines_samples*bands*sizeof(float));
        switch(dataType)
        {
            case 2:
                tipo_short_int = (short int*)malloc(lines_samples*bands*sizeof(short int));
                fread(tipo_short_int,1,(sizeof(short int)*lines_samples*bands),fp);
                for(i=0; i<lines_samples * bands; i++)
                    tipo_float[i]=(float)tipo_short_int[i];
                free(tipo_short_int);
                break;

            case 4:
                fread(tipo_float,1,(sizeof(float)*lines_samples*bands),fp);
                break;

            case 5:
                tipo_double = (double*)malloc(lines_samples*bands*sizeof(double));
                fread(tipo_double,1,(sizeof(double)*lines_samples*bands),fp);
                for(i=0; i<lines_samples * bands; i++)
                    tipo_float[i]=(float)tipo_double[i];
                free(tipo_double);
                break;

            case 12:
                tipo_uint = (unsigned int*)malloc(lines_samples*bands*sizeof(unsigned int));
                fread(tipo_uint,1,(sizeof(unsigned int)*lines_samples*bands),fp);
                for(i=0; i<lines_samples * bands; i++)
                    tipo_float[i]=(float)tipo_uint[i];
                free(tipo_uint);
                break;

        }
        fclose(fp);

        if(interleave == NULL)
            op = 0;
        else
        {
            if(strcmp(interleave, "bsq") == 0) op = 0;
            if(strcmp(interleave, "bip") == 0) op = 1;
            if(strcmp(interleave, "bil") == 0) op = 2;
        }


        switch(op)
        {
            case 0:
                for(i=0; i<lines*samples*bands; i++)
                    image[i] = tipo_float[i];
                break;

            case 1:
                for(i=0; i<bands; i++)
                    for(j=0; j<lines*samples; j++)
                        image[i*lines*samples + j] = tipo_float[j*bands + i];
                break;

            case 2:
                for(i=0; i<lines; i++)
                    for(j=0; j<bands; j++)
                        for(k=0; k<samples; k++)
                            image[j*lines*samples + (i*samples+k)] = tipo_float[k+samples*(i*bands+j)];
                break;
        }
        free(tipo_float);
        return 0;
    }
    return -2;
}

/*
 * Author: Luis Ignacio Jimenez Gil
 * Centre: Universidad de Extremadura
 * */
int writeValueResults(char* filename, int result)
{
    FILE *fp;
    int out = result;
    if ((fp=fopen(filename,"wb"))!=NULL)
    {
        fseek(fp,0L,SEEK_SET);
        fwrite(&out,1,sizeof(int),fp);
        fclose(fp);
    }
    else return -1;

    return 0;
}

/*
 * Author: Luis Ignacio Jimenez Gil
 * Centre: Universidad de Extremadura
 * */
int main(int argc, char* argv[])
{

	/*
	 * PARAMETERS:
	 *
	 * argv[1]: Image filename
	 * argv[2]: Approximation value
	 * argv[3]: Output Result File
	 *
	 * */

	if(argc != 4)
	{
		printf("EXECUTION ERROR VD Iterative: Parameters are not correct.");
		printf("./VD [Image Filename] [Approximation] [Output Result File]");
		fflush(stdout);
		exit(-1);
	}
 
 struct timeval t0,tfin;
float t_sec, t_usec, secsVD=0.0;

  int i, j, N;
  //float mean;

  double sigmaSquareTest;
  double sigmaTest;
  double TaoTest;

  double *meanSpect;
  double *Cov;
  double *Corr;
  double *CovEigVal;
  double *CorrEigVal;
  double *U;
  double *VT;

	//READ IMAGE
	char cad[MAXCAD];
	int lines = 0, samples= 0, bands= 0, dataType= 0, byteOrder = 0;
	char *interleave, *waveUnit;
	interleave = (char*)malloc(MAXCAD*sizeof(char));
	waveUnit = (char*)malloc(MAXCAD*sizeof(char));

	strcpy(cad,argv[1]); // Second parameter: Header file:
	strcat(cad,".hdr");
	int error = readHeader1(cad, &lines, &samples, &bands, &dataType, interleave, &byteOrder, waveUnit);
	if(error != 0)
	{
		printf("EXECUTION ERROR VD Iterative: Error 1 reading header file: %s.", cad);
		fflush(stdout);
		exit(-1);
	}

	double* wavelength = (double*)malloc(bands*sizeof(double));
	strcpy(cad,argv[1]);
	strcat(cad,".hdr");
	error = readHeader2(cad, wavelength);
	if(error != 0)
	{
		printf("EXECUTION ERROR VD Iterative: Error 2 reading header file: %s.", cad);
		fflush(stdout);
		exit(-1);
	}
	double *image = (double*)malloc(lines*samples*bands*sizeof(double));
	error = loadImage(argv[1], image, lines, samples, bands, dataType, interleave);
	if(error != 0)
	{
		printf("EXECUTION ERROR: Error reading image file: %s.", argv[1]);
		fflush(stdout);
		exit(-1);
	}

    N=lines*samples;

    float* mean = (float*)malloc(bands*sizeof(float));
    
	//COVARIANCE
    meanSpect		= (double*) malloc(bands * sizeof(double));
    Cov			= (double*) malloc(bands * bands * sizeof(double));
    Corr			= (double*) malloc(bands * bands * sizeof(double));
    CovEigVal		= (double*) malloc(bands * sizeof(double));
    CorrEigVal	= (double*) malloc(bands * sizeof(double));
    U		= (double*) malloc(bands * bands * sizeof(double));
    VT	= (double*) malloc(bands * bands * sizeof(double));

    cudaSetDevice(0); // 0 es RTX 3090 y 1 es V100

    cudaStream_t stream;
    cudaStreamCreate(&stream);
    
    cublasHandle_t handle_gemm = NULL;
    cublasStatus_t cublas_status = CUBLAS_STATUS_SUCCESS;
    cublasCreate(&handle_gemm);
    assert(CUBLAS_STATUS_SUCCESS == cublas_status);
    
    cusolverStatus_t cusolver_status = CUSOLVER_STATUS_SUCCESS;

    cusolverDnHandle_t cusolverHandle = NULL;
    cusolver_status = cusolverDnCreate(&cusolverHandle);
    assert(CUSOLVER_STATUS_SUCCESS == cusolver_status);
    
    int* count = (int*)malloc(FPS * sizeof(int));

    int lwork = MAX(1,MAX(3*MIN(bands, bands)+MAX(bands,bands),5*MIN(bands,bands)));
    int *info;
    
   	double *rwork  = (double*)malloc(lwork*sizeof(double));
    double *work = NULL;
    
    int cublas_error;
    
    int limit = sizeof(int);

    gettimeofday(&t0,NULL); // Measure the time - Start ATDCA algorithm

#pragma acc data copyin(meanSpect[0:bands], Cov[0:bands*bands], Corr[0:bands*bands], CovEigVal[0:bands], CorrEigVal[0:bands], U[0:bands*bands], VT[0:bands*bands], image[0:lines*samples*bands], info[0:limit], rwork[0:lwork], mean[0:bands], work[0:lwork]) \
                 copyout(count[0:FPS])
{

    #pragma acc parallel loop
    for(i=0; i<bands; i++)
    {
      mean[i]=0;
    
      #pragma acc loop seq
      for(j=0; j<N; j++)
        mean[i]+=(image[(i*N)+j]);
    
      mean[i]/=N;
      meanSpect[i]=mean[i];
    
      #pragma acc loop
      for(j=0; j<N; j++)
        image[(i*N)+j]=image[(i*N)+j]-mean[i];
    }
    
    double alpha = (double)1/N, beta = 0;
    
    #pragma acc host_data use_device(image, Cov)
    {
      cublas_error = cublasDgemm(handle_gemm,CUBLAS_OP_T, CUBLAS_OP_N, bands, bands, N, &alpha, image, N, image, N, &beta, Cov, bands);
      
      if( cublas_error != CUBLAS_STATUS_SUCCESS )
      {
        printf( "failed cuBLAS execution %d\n", cublas_error );
        exit(1);
      }
    }
    
    cublasGetStream(handle_gemm, &stream);
    cudaStreamSynchronize(stream);
    
    //CORRELATION
    #pragma acc parallel loop collapse(2)
    for(j=0; j<bands; j++)
      for(i=0; i<bands; i++)
      	Corr[(i*bands)+j] = Cov[(i*bands)+j]+(meanSpect[i] * meanSpect[j]);
    
    //SVD
    cusolver_status = cusolverDnDgesvd_bufferSize(
                      cusolverHandle,
                      bands,
                      bands,
                      &lwork);
    assert(cusolver_status == CUSOLVER_STATUS_SUCCESS);
    cudaMalloc((void**)&work , sizeof(double)*lwork);

    #pragma acc host_data use_device(Cov, CovEigVal, U, VT, info, rwork)
    {
      cusolver_status = cusolverDnDgesvd(cusolverHandle,'N', 'N', bands, bands, Cov, bands, CovEigVal, U, bands, VT, bands, work, lwork, rwork, info);
      
      if( cusolver_status != CUSOLVER_STATUS_SUCCESS )
      {
        printf( "failed cuSOLVER execution %d\n", cusolver_status );
        exit(1);
      }
    }

    #pragma acc host_data use_device(Corr, CorrEigVal, U, VT, info, rwork)
    {
      cusolver_status = cusolverDnDgesvd(cusolverHandle,'N', 'N', bands, bands, Corr, bands, CorrEigVal, U, bands, VT, bands, work, lwork, rwork, info);
      
      if( cusolver_status != CUSOLVER_STATUS_SUCCESS )
      {
        printf( "failed cuSOLVER execution %d\n", cusolver_status );
        exit(1);
      }
    }
    
    //ESTIMATION
    double e;
    
    #pragma acc serial loop
    for(i=0; i<FPS; i++) count[i] = 0;
    
    #pragma acc kernels
    #pragma acc loop seq
    for(i=0; i<bands; i++)
    {
    
    	sigmaSquareTest = (CovEigVal[i]*CovEigVal[i]+CorrEigVal[i]*CorrEigVal[i])*2/samples/lines;
    	sigmaTest = sqrt(sigmaSquareTest);
     
      #pragma acc loop seq
    	for(j=1;j<=FPS;j++)
      {
        	switch(j)
        	{
    				case 1: e = 0.906193802436823;
    				break;
    				case 2: e = 1.644976357133188;
    				break;
    				case 3: e = 2.185124219133003;
    				break;
    				case 4: e = 2.629741776210312;
    				break;
    				case 5: e = 3.015733201402701;
    				break;
        	}
         
          TaoTest = sqrt(2) * sigmaTest * e;
  
          if((CorrEigVal[i]-CovEigVal[i]) > TaoTest) count[j-1]++;
       }
    }
}
    int res = count[atoi(argv[2])-1];
    printf("res = %d\n", res);

    gettimeofday(&tfin,NULL); //Measure the time - End ATDCA algorithm

    //CALCULATE THE TIME FOR VD ALGORITHM
    t_sec  = (float)  (tfin.tv_sec - t0.tv_sec);
    t_usec = (float)  (tfin.tv_usec - t0.tv_usec);
    secsVD = t_sec + t_usec/1.0e+6;   

    //OUTPUT THE TOTAL TIME
    printf("VD IT algorithm:\t%.5f seconds\n", secsVD);

	error = writeValueResults(argv[3], res);
	if(error != 0)
	{
		printf("EXECUTION ERROR VD Iterative: Error writing results file: %s.", argv[3]);
		fflush(stdout);
		exit(-1);
	}

    free(meanSpect);
    free(count);
    free(Cov);
    free(Corr);
    free(CovEigVal);
    free(CorrEigVal);
    free(U);
    free(VT);
    free(image);
    free(interleave);
    free(waveUnit);
    free(wavelength);
    free(mean);
    free(rwork);
    cublasDestroy(handle_gemm);
    cusolverDnDestroy(cusolverHandle);

	return 0;
}
