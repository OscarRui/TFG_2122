#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include <limits>
#include <string.h>
#include "oneapi/mkl.hpp"
#include <omp.h>

using namespace std;

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

#define MAXLINE 200
#define MAXCAD 200
#define EPSILON 1.11e-16

float time_diff(struct timeval *start2, struct timeval *end2)
{
  return (end2->tv_sec - start2->tv_sec) + 1e-6*(end2->tv_usec - start2->tv_usec);
}

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
    char line[MAXLINE] ="";
    char value [MAXLINE] = "";

    if ((fp=fopen(filename,"rt"))!=NULL)
    {
        fseek(fp,0L,SEEK_SET);
        while(fgets(line, MAXLINE, fp)!=NULL)
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
    char line[MAXLINE] = "";
    char value [MAXLINE] = "";

    if ((fp=fopen(filename,"rt"))!=NULL)
    {
        fseek(fp,0L,SEEK_SET);
        while(fgets(line, MAXLINE, fp)!=NULL)
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
int readValueResults(char* filename)
{
    FILE *fp;
    int in, error;
    int value;
    if ((fp=fopen(filename,"rb"))!=NULL)
    {
        fseek(fp,0L,SEEK_SET);
        error = fread(&value,1,sizeof(int),fp);
        if(error != sizeof(int)) in = -1;
        else in = (int)value;
        fclose(fp);
    }
    else return -1;

    return in;
}

/*
 * Author: Luis Ignacio Jimenez
 * Centre: Universidad de Extremadura
 * */
int writeResult(double *image, const char* filename, int lines, int samples, int bands, int dataType, char* interleave)
{

	short int *imageSI;
	float *imageF;
	double *imageD;

	int i,j,k,op;

	if(interleave == NULL)
		op = 0;
	else
	{
		if(strcmp(interleave, "bsq") == 0) op = 0;
		if(strcmp(interleave, "bip") == 0) op = 1;
		if(strcmp(interleave, "bil") == 0) op = 2;
	}

	if(dataType == 2)
	{
		imageSI = (short int*)malloc(lines*samples*bands*sizeof(short int));

        switch(op)
        {
			case 0:
				for(i=0; i<lines*samples*bands; i++)
					imageSI[i] = (short int)image[i];
				break;

			case 1:
				for(i=0; i<bands; i++)
					for(j=0; j<lines*samples; j++)
						imageSI[j*bands + i] = (short int)image[i*lines*samples + j];
				break;

			case 2:
				for(i=0; i<lines; i++)
					for(j=0; j<bands; j++)
						for(k=0; k<samples; k++)
							imageSI[i*bands*samples + (j*samples + k)] = (short int)image[j*lines*samples + (i*samples + k)];
				break;
        }

	}

	if(dataType == 4)
	{
		imageF = (float*)malloc(lines*samples*bands*sizeof(float));
        switch(op)
        {
			case 0:
				for(i=0; i<lines*samples*bands; i++)
					imageF[i] = (float)image[i];
				break;

			case 1:
				for(i=0; i<bands; i++)
					for(j=0; j<lines*samples; j++)
						imageF[j*bands + i] = (float)image[i*lines*samples + j];
				break;

			case 2:
				for(i=0; i<lines; i++)
					for(j=0; j<bands; j++)
						for(k=0; k<samples; k++)
							imageF[i*bands*samples + (j*samples + k)] = (float)image[j*lines*samples + (i*samples + k)];
				break;
        }
	}

	if(dataType == 5)
	{
		imageD = (double*)malloc(lines*samples*bands*sizeof(double));
        switch(op)
        {
			case 0:
				for(i=0; i<lines*samples*bands; i++)
					imageD[i] = image[i];
				break;

			case 1:
				for(i=0; i<bands; i++)
					for(j=0; j<lines*samples; j++)
						imageD[j*bands + i] = image[i*lines*samples + j];
				break;

			case 2:
				for(i=0; i<lines; i++)
					for(j=0; j<bands; j++)
						for(k=0; k<samples; k++)
							imageD[i*bands*samples + (j*samples + k)] = image[j*lines*samples + (i*samples + k)];
				break;
        }
	}

    FILE *fp;
    if ((fp=fopen(filename,"wb"))!=NULL)
    {
        fseek(fp,0L,SEEK_SET);
	    switch(dataType)
	    {
	    case 2: fwrite(imageSI,1,(lines*samples*bands * sizeof(short int)),fp); free(imageSI); break;
	    case 4: fwrite(imageF,1,(lines*samples*bands * sizeof(float)),fp); free(imageF); break;
	    case 5: fwrite(imageD,1,(lines*samples*bands * sizeof(double)),fp); free(imageD); break;
	    }
	    fclose(fp);


	    return 0;
    }

    return -3;
}

/*
 * Author: Luis Ignacio Jimenez
 * Centre: Universidad de Extremadura
 * */
int writeHeader(char* filename, int lines, int samples, int bands, int dataType,
		char* interleave, int byteOrder, char* waveUnit, double* wavelength)
{
    FILE *fp;
    if ((fp=fopen(filename,"wt"))!=NULL)
    {
		fseek(fp,0L,SEEK_SET);
		fprintf(fp,"ENVI\ndescription = {\nExported from MATLAB}\n");
		if(samples != 0) fprintf(fp,"samples = %d", samples);
		if(lines != 0) fprintf(fp,"\nlines   = %d", lines);
		if(bands != 0) fprintf(fp,"\nbands   = %d", bands);
		if(dataType != 0) fprintf(fp,"\ndata type = %d", dataType);
		if(interleave != NULL) fprintf(fp,"\ninterleave = %s", interleave);
		if(byteOrder != 0) fprintf(fp,"\nbyte order = %d", byteOrder);
		if(waveUnit != NULL) fprintf(fp,"\nwavelength units = %s", waveUnit);
		if(waveUnit != NULL)
		{
			fprintf(fp,"\nwavelength = {\n");
			for(int i=0; i<bands; i++)
			{
				if(i==0) fprintf(fp, "%f", wavelength[i]);
				else
					if(i%3 == 0) fprintf(fp, ", %f\n", wavelength[i]);
					else fprintf(fp, ", %f", wavelength[i]);
			}
			fprintf(fp,"}");
		}
		fclose(fp);
		return 0;
    }
    return -3;
}


/*
 * Author: Luis Ignacio Jimenez Gil
 * Centre: Universidad de Extremadura
 * */
void mean(double* matrix, int rows, int cols, int dim, double* out)
{
	int i,j;

	if(dim == 1)
	{
		for(i=0; i<cols; i++) out[i] = 0;

		for(i=0; i<cols; i++)
			for(j=0; j<rows; j++)
				out[i] += matrix[j*cols + i];

		for(i=0; i<cols; i++) out[i] = out[i]/cols;
	}
	else
	{
		for(i=0; i<rows; i++) out[i] = 0;

		for(i=0; i<rows; i++)
			for(j=0; j<cols; j++)
				out[i] += matrix[i*cols + j];

		for(i=0; i<rows; i++) out[i] = out[i]/rows;

	}
}

/* Siconos-Numerics, Copyright INRIA 2005-2011.
 * Siconos is a program dedicated to modeling, simulation and control
 * of non smooth dynamical systems.
 * Siconos is a free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * Siconos is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Siconos; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Contact: Vincent ACARY, siconos-team@lists.gforge.inria.fr
*/
/*
void pinv(double * A, int n, int m)
{
	int dimS = MIN(n,m);
	int LDU = n;
	int LDVT = m;
	int i,j;
	double alpha = 1, beta = 0;
	int lwork  = MAX(1,MAX(3*MIN(m, n)+MAX(m,n),5*MIN(m,n))) , info;
	double *work  = (double*)malloc(lwork*sizeof(double));

	double *S = (double*) malloc(dimS * sizeof(double));//eigenvalues
	double *U = (double*) malloc(LDU * n * sizeof(double));//eigenvectors
	double *VT = (double*) malloc(LDVT * m * sizeof(double));//eigenvectors

	dgesvd_("S", "S", &n, &m, A, &n, S, U, &LDU, VT, &LDVT, work, &lwork, &info);

	double maxi = S[0];
	for(i=1; i<dimS; i++)
		if(maxi<S[i]) maxi = S[i];

	double tolerance = EPSILON*MAX(n,m)*maxi;

	int rank = 0;
	for (i=0; i<dimS; i++)
	{
		if (S[i] > tolerance)
		{
			rank ++;
			S[i] = 1.0 / S[i];
		}
	}

	//Compute the pseudo inverse 
	//Costly version with full DGEMM
	double * Utranstmp = (double*)malloc(n * m * sizeof(double));
	for (i=0; i<dimS; i++)
		for (j=0;  j<n; j++) Utranstmp[i + j * m] = S[i] * U[j + i * n];

	for (i=dimS;  i<m; i++)
		for (j=0;  j<n; j++) Utranstmp[i + j * m] = 0.0;

	dgemm_("T", "N", &m, &n, &m, &alpha, VT, &m, Utranstmp, &m, &beta, A, &m);

	free(work);
	free(U);
	free(VT);
	free(Utranstmp);
	free(S);
}
*/

/*
 * Author: Luis Ignacio Jimenez
 * Centre: Universidad de Extremadura
 * */
int main(int argc, char *argv[])//(double *image, int lines, int samples, int bands, int targets, double SNR, double *endmembers)
{

	/*
	 * PARAMETERS
	 * argv[1]: Input image file
	 * argv[2]: number of endmembers to be extracted
	 * argv[3]: Signal noise ratio (SNR)
	 * argv[4]: estimated endmember signatures obtained
	 * */

	if(argc != 5)
	{
		printf("EXECUTION ERROR VCA Iterative: Parameters are not correct.");
		printf("./VCA [Image Filename] [Number of endmembers] [SNR] [Output endmembers]");
		fflush(stdout);
		exit(-1);
	}
	// Input parameters:
	char image_filename[MAXCAD];
	char header_filename[MAXCAD];

	strcpy(image_filename,argv[1]);
	strcpy(header_filename,argv[1]);
	strcat(header_filename,".hdr");

	// Cola
	sycl::queue my_queue{sycl::cpu_selector{}};

	std::cout << "Device: "
			<< my_queue.get_device().get_info<sycl::info::device::name>()
			<< std::endl;
	int *info;

	int lines = 0, samples= 0, bands= 0, dataType= 0, byteOrder = 0;
	char *interleave, *waveUnit;
	interleave = (char*)malloc(MAXCAD*sizeof(char));
	waveUnit = (char*)malloc(MAXCAD*sizeof(char));

	// Load image
	int error = readHeader1(header_filename, &lines, &samples, &bands, &dataType, interleave, &byteOrder, waveUnit);
	if(error != 0)
	{
		printf("\nEXECUTION ERROR VCA Iterative: Error 1 reading header file: %s.", header_filename);
		fflush(stdout);
		exit(-1);
	}

	double* wavelength = (double*)malloc(bands*sizeof(double));

	strcpy(header_filename,argv[1]); // Second parameter: Header file:
	strcat(header_filename,".hdr");
	error = readHeader2(header_filename, wavelength);
	if(error != 0)
	{
		printf("\nEXECUTION ERROR VCA Iterative: Error 2 reading header file: %s.", header_filename);
		fflush(stdout);
		exit(-1);
	}

	double *image_vector = (double *) malloc (sizeof(double)*(lines*samples*bands));

	error = loadImage(argv[1],image_vector, lines, samples, bands, dataType, interleave);
	if(error != 0)
	{
		printf("\nEXECUTION ERROR VCA Iterative: Error reading image file: %s.", argv[1]);
		fflush(stdout);
		exit(-1);
	}

	//***TARGETS VALUE OR LOAD VALUE****
	int targets;
	if(strstr(argv[2], "/") == NULL)
		targets = atoi(argv[2]);
	else
	{
		targets = readValueResults(argv[2]);
		fflush(stdout);
		if(targets == -1)
		{
			printf("EXECUTION ERROR IEA Iterative: Targets was not set correctly file: %s.", argv[2]);
			fflush(stdout);
			exit(-1);
		}
	}
	//**********************************

	//START CLOCK***************************************
  	struct timeval start;
 	struct timeval end;
  	gettimeofday(&start, NULL);
	//**************************************************
	int i, j, lines_samples = lines*samples;
	double alpha =1, beta = 0;

	double *Ud = (double*) calloc(bands * targets , sizeof(double));
	double *x_p = (double*) calloc(lines_samples * targets , sizeof(double));
	double *y = (double*) calloc(lines_samples * targets , sizeof(double));
	double *R_o = (double*)calloc(bands*lines_samples,sizeof(double));
	double *r_m = (double*)calloc(bands,sizeof(double));
	double *svdMat = (double*)calloc(bands*bands,sizeof(double));
	double *D = (double*) calloc(bands , sizeof(double));//eigenvalues
	double *U = (double*) calloc(bands * bands , sizeof(double));//eigenvectors
	double *VT = (double*) calloc(bands * bands , sizeof(double));//eigenvectors
	double *endmembers = (double*) calloc(targets * bands , sizeof(double));
	double *Rp = (double*)calloc(bands*lines_samples,sizeof(double));
	double *u = (double*)calloc(targets,sizeof(double));
	double *sumxu = (double*)calloc(lines_samples,sizeof(double));
	int* index = (int*)calloc(targets,sizeof(int));
	double *w = (double*)calloc(targets,sizeof(double));
	double *A = (double*)calloc(targets*targets,sizeof(double));
	double *A2 = (double*)calloc(targets*targets,sizeof(double));
	double *aux = (double*)calloc(targets*targets,sizeof(double));
	double *f = (double*)calloc(targets,sizeof(double));

	double SNR = atof(argv[3]), SNR_es;
	double sum1, sum2, powery, powerx, mult = 0;
	

	double* S_pinv       = (double*)calloc(targets,sizeof(double));
	double* U_pinv       = (double*)calloc(targets*targets,sizeof(double));
	double* VT_pinv      = (double*)calloc(targets*targets,sizeof(double));
	double* Utranstmp    = (double*)calloc(targets*targets,sizeof(double));
	double maxi;
	double tolerance;
	int rank;

	#pragma omp teams distribute
	for(i=0; i<bands; i++)
	{
		#pragma omp single
		for(j=0; j<lines_samples; j++)
			r_m[i] += image_vector[i*lines_samples+j];

		r_m[i] /= lines_samples;

		#pragma omp parallel for
		for(j=0; j<lines_samples; j++)
			R_o[i*lines_samples+j] = image_vector[i*lines_samples+j] - r_m[i];
	}

	cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, bands, bands, lines_samples, alpha, R_o, lines_samples, R_o, lines_samples, beta, svdMat, bands);

	#pragma omp teams distribute parallel for
	for(i=0; i<bands*bands; i++) svdMat[i] /= lines_samples;

	double superb[bands-1];
	LAPACKE_dgesvd(LAPACK_COL_MAJOR, 'S', 'S', bands, bands, svdMat, bands, D, U, bands, VT, bands, superb);

	#pragma omp teams distribute parallel for collapse(2)
	for(i=0; i<bands; i++)
		for(j=0; j<targets; j++)
			Ud[i*targets +j] = VT[i*bands +j];

	cblas_dgemm(CblasColMajor, CblasNoTrans, CblasTrans, targets, lines_samples, bands, alpha, Ud, targets, R_o, lines_samples, beta, x_p, targets);

	sum1 =0;
	sum2 = 0;
	mult = 0;

	for(i=0; i<lines_samples*bands; i++)
	{
		sum1 += pow(image_vector[i],2);
		if(i < lines_samples*targets) sum2 += pow(x_p[i],2);
		if(i < bands) mult += pow(r_m[i],2);
	}

	powery = sum1 / lines_samples;
	powerx = sum2 / lines_samples + mult;

	SNR_es = 10 * log10((powerx - targets / bands * powery) / (powery - powerx));


	if(SNR == 0) SNR = SNR_es;
	double SNR_th = 15 + 10*log10(targets), c;

	if(SNR < SNR_th)
	{
		#pragma omp teams distribute parallel for collapse(2)
		for(i=0; i<bands; i++)
			for(j=0; j<targets; j++)
				if(j<targets-1) Ud[i*targets +j] = VT[i*bands +j];
				else  Ud[i*targets +j] = 0;

		sum1 = 0;

		#pragma omp teams distribute parallel for
		for(i=0; i<targets; i++)
		{
			for(j=0; j<lines_samples; j++)
			{
				if(i == (targets-1)) x_p[i*lines_samples+j] = 0;
				u[i] += pow(x_p[i*lines_samples+j], 2);
			}

			if(sum1 < u[i]) sum1 = u[i];
		}

		c = sqrt(sum1);

		cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, bands, lines_samples, targets, alpha, Ud, targets, x_p, targets, beta, Rp, bands);

		#pragma omp teams distribute parallel for collapse(2)
		for(i=0; i<bands; i++)
			for(j=0; j<lines_samples; j++)
				Rp[i*lines_samples+j] += r_m[i];

		#pragma omp teams distribute parallel for collapse(2)
		for(i=0; i<targets; i++)
			for(j=0; j<lines_samples; j++)
				if(i<targets-1) y[i*lines_samples+j] = x_p[i*lines_samples+j];
				else y[i*lines_samples+j] = c;
	}
	else
	{

		cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, bands, bands, lines_samples, alpha, image_vector, lines_samples, image_vector, lines_samples, beta, svdMat, bands);

		#pragma omp teams distribute parallel for
		for(i=0; i<bands*bands; i++) svdMat[i] /= lines_samples;

		LAPACKE_dgesvd(LAPACK_COL_MAJOR, 'S', 'S', bands, bands, svdMat, bands, D, U, bands, VT, bands, superb);

		#pragma omp teams distribute parallel for collapse(2)
		for(i=0; i<bands; i++)
			for(j=0; j<targets; j++)
				Ud[i*targets +j] = VT[i*bands +j];

		cblas_dgemm(CblasColMajor, CblasNoTrans, CblasTrans, targets, lines_samples, bands, alpha, Ud, targets, image_vector, lines_samples, beta, x_p, targets);

		cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, bands, lines_samples, targets, alpha, Ud, targets, x_p, targets, beta, Rp, bands);

		#pragma omp teams distribute
		for(i=0; i<targets; i++)
		{
			#pragma omp single
			for(j=0; j<lines_samples; j++)
				u[i] += x_p[i*lines_samples+j];

			#pragma omp parallel for
			for(j=0; j<lines_samples; j++)
				y[i*lines_samples+j] = x_p[i*lines_samples+j] * u[i];
		}

		#pragma omp teams distribute parallel for collapse(2)
		for(i=0; i<lines_samples; i++) {
			#pragma omp parallel for
			for(j=0; j<targets; j++)
				sumxu[i] += y[j*lines_samples+i];
		}

		#pragma omp teams distribute parallel for collapse(2)
		for(i=0; i<targets; i++)
			for(j=0; j<lines_samples; j++)
				y[i*lines_samples+j] /= sumxu[j];
	}

	srand(time(NULL));

	int lmax = std::numeric_limits<int>::max(), one = 1;
	A[(targets-1)*targets] = 1;

	for(i=0; i<targets; i++)
	{
		#pragma omp teams distribute parallel for
		for(j=0; j<targets; j++)
		{
			w[j] = 16000 % lmax; // Cambiamos el valor rand() por un valor fijo 16000
			w[j] /= lmax;
		}

		#pragma omp teams distribute parallel for
		for(j=0; j<targets*targets; j++) A2[j] = A[j];

		/*
		*
		*
		*
		* Implementacion directa de PINV
		*
		*
		*
		*/
		//pinv(A2, targets, targets);
		int i_pinv,j_pinv;

		double superb_pinv[targets-1];
		LAPACKE_dgesvd(LAPACK_COL_MAJOR, 'S', 'S', targets, targets, A2, targets, S_pinv, U_pinv, targets, VT_pinv, targets, superb_pinv);

		maxi = S_pinv[0];

		for(i_pinv=1; i_pinv<targets; i_pinv++)
    	{
			if(maxi<S_pinv[i_pinv]) maxi = S_pinv[i_pinv];
    	}

		tolerance = EPSILON*MAX(targets,targets)*maxi;
      	rank = 0;

		for (i_pinv=0; i_pinv<targets; i_pinv++)
		{
			if (S_pinv[i_pinv] > tolerance)
			{
				rank += 1;
				S_pinv[i_pinv] = 1.0 / S_pinv[i_pinv];
			}
		}

		// Compute the pseudo inverse
		// Costly version with full DGEMM
		#pragma omp teams distribute parallel for collapse(2)
		for (i_pinv=0; i_pinv<targets; i_pinv++)
			for (j_pinv=0;  j_pinv<targets; j_pinv++) 
				Utranstmp[i_pinv + j_pinv * targets] = S_pinv[i_pinv] * U_pinv[j_pinv + i_pinv * targets];

		cblas_dgemm(CblasColMajor, CblasTrans, CblasNoTrans, targets, targets, targets, alpha, VT_pinv, targets, Utranstmp, targets, beta, A2, targets);


		/********************************************************************************************************************************************************************/

		cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, targets, targets, targets, alpha, A2, targets, A, targets, beta, aux, targets);

		cblas_dgemm(CblasColMajor, CblasNoTrans, CblasNoTrans, targets, one, targets, alpha, aux, targets, w, targets, beta, f, targets);

	    sum1 = 0;
	    for(j=0; j<targets; j++)
	    {

	    	f[j] = w[j] - f[j];
	    	sum1 += pow(f[j],2);
	    }

	    for(j=0; j<targets; j++) f[j] /= sqrt(sum1);

		cblas_dgemm(CblasColMajor, CblasNoTrans, CblasTrans, one, lines_samples, targets, alpha, f, one, y, lines_samples, beta, sumxu, one);

	    sum2 = 0;

	    for(j=0; j<lines_samples; j++)
	    {
	    	if(sumxu[j] < 0) sumxu[j] *= -1;
	    	if(sum2 < sumxu[j])
	    	{
	    		sum2 = sumxu[j];
	    		index[i] = j;
	    	}
	    }

		#pragma omp teams distribute parallel for
	    for(j=0; j<targets; j++)
	    	A[j*targets + i] = y[j*lines_samples+index[i]];

		#pragma omp teams distribute parallel for
	    for(j=0; j<bands; j++)
	    	endmembers[j*targets+ i] = Rp[j+bands * index[i]];
	}

	//END CLOCK*****************************************
  	gettimeofday(&end, NULL);
  	printf("Time spent: %0.8f sec\n",time_diff(&start, &end));
	fflush(stdout);
	//**************************************************

	strcpy(image_filename, argv[4]);
	strcpy(header_filename, image_filename);
	strcat(header_filename, ".hdr");
	error = writeHeader(header_filename, targets, 1, bands, dataType, interleave, byteOrder, waveUnit, wavelength);
	if(error != 0)
	{
		printf("EXECUTION ERROR VCA Iterative: Error writing header file: %s.", header_filename);
		fflush(stdout);
		exit(-1);
	}
	error = writeResult(endmembers, argv[4], targets, 1, bands, dataType, interleave);
	if(error != 0)
	{
		printf("EXECUTION ERROR VCA Iterative: Error writing image file: %s.", argv[3]);
		fflush(stdout);
		exit(-1);
	}

	free(sumxu);
	free(aux);
	free(f);
	free(A);
	free(A2);
	free(w);
	free(index);
	free(y);
	free(u);
	free(Rp);
	free(x_p);
	free(R_o);
	free(svdMat);
	free(D);
	free(U);
	free(VT);
	free(r_m);
	free(Ud);

	free(image_vector);
	free(wavelength);
	free(interleave);
	free(waveUnit);
	free(endmembers);

	free(U_pinv);
	free(VT_pinv);
	free(Utranstmp);
	free(S_pinv);


	return 0;
}
