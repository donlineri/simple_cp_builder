#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <setoper.h>
#include <cdd.h>
#include "shader_s.h"
#include "space.hpp"

enum {
	max_pathname_in = 255,
	max_pathname_out = 255,
	max_length_line = 255,
};

void draw_cp(unsigned int VAO, int count_vertices, Shader *shader)
{
	glm::mat4 model;
	unsigned int modelLoc;
	model = glm::mat4(1.0f);
	modelLoc = glGetUniformLocation(shader->ID, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, count_vertices);
}

void swap_double(double *a, double *b)
{
	double t;
	t = *a;
	*a = *b;
	*b = t;
}

void swap_int(int *a, int *b)
{
	int t;
	t = *a;
	*a = *b;
	*b = t;
}

void order_vertices(double **cp_vertices, double (**g)[1],
		unsigned long rowsize, unsigned long colsize)
{
  unsigned long i, j;
	double centroidX = 0.0, centroidY = 0.0, *cp_angle_vertices;
	int *cp_number_vertices;
	cp_angle_vertices = (double *) malloc(sizeof(double)*rowsize);
	cp_number_vertices = (int *) malloc(sizeof(int)*rowsize);
	*cp_vertices = (double *) malloc(sizeof(double)*(rowsize * 6));
	for(i = 0; i < rowsize; i++) {
		centroidX += g[i][1][0];
		centroidY += g[i][2][0];
	}
	//printf("rowsize: %ld\n", rowsize);
	//printf("centroidX = %lf\ncentroidY = %lf\n", centroidX, centroidY);
	centroidX /= rowsize;
	centroidY /= rowsize;
	//printf("centroidX = %lf\ncentroidY = %lf\n", centroidX, centroidY);
	for(i = 0; i < rowsize; i++) {
		cp_number_vertices[i] = i;
		cp_angle_vertices[i] = atan2(g[i][2][0] - centroidY, g[i][1][0] - centroidX);
	}
	/*
	for(i = 0; i < rowsize; i++)
		printf("vertex %d angle %lf \n", cp_number_vertices[i], cp_angle_vertices[i]);
		*/
	for(j = rowsize; j > 1; j--)
		for(i = 1; i < j; i++)
			if(cp_angle_vertices[i-1] > cp_angle_vertices[i]) {
				swap_double(&cp_angle_vertices[i-1], &cp_angle_vertices[i]);
				swap_int(&cp_number_vertices[i-1], &cp_number_vertices[i]);
			}
	/*
	for(i = 0; i < rowsize; i++)
		printf("vertex %d angle %lf \n", cp_number_vertices[i], cp_angle_vertices[i]);
		*/
	/*
	int frontFaceMode;
	//glFrontFace(GL_CW);
	glGetIntegerv(GL_FRONT_FACE, &frontFaceMode);
	if(frontFaceMode == GL_CW)
		printf("glFrontFace is GL_CW (clockwise)\n");
	else if(frontFaceMode == GL_CCW)
		printf("glFrontFace is GL_CCW (counter clockwise)\n");
	else
		printf("glFrontFace has an unexpected value\n");
		*/
  for(i = 0; i < rowsize; i++) {
    for(j = 0; j < colsize; j++)
      printf("%lf ", g[i][j][0]);
		(*cp_vertices)[i*6] = g[cp_number_vertices[i]][1][0];
		(*cp_vertices)[i*6+1] = g[cp_number_vertices[i]][2][0];
		if(g[i][0][0] != 1.0) {
			fprintf(stderr, "Error: extreme ray found\n");
			exit(1);
		}
		(*cp_vertices)[i*6+2] = 0.0;
		(*cp_vertices)[i*6+3] = 1.0;
		(*cp_vertices)[i*6+4] = 0.5;
		(*cp_vertices)[i*6+5] = 0.2;
    printf("\n");
  }
	free(cp_angle_vertices);
	free(cp_number_vertices);
}

void open_problem_file(FILE **problem_file, char *name)
{
	printf("Input file: ");
	fgets(name, max_pathname_in, stdin);
	name[strlen(name)-1] = '\0';
	*problem_file = fopen(name, "r");
	if(!*problem_file) {
		perror(name);
		exit(1);
	}
}

void open_result_file(FILE **result_file, char *name)
{
	time_t timep;
	char *date;
	timep = time(NULL);
	date = asctime(localtime(&timep));
	date[strlen(date)-1] = '\0';
	sprintf(name, "/tmp/simple_cp_builder_%.100s.ine", date);
	*result_file = fopen(name,  "w");
	if(!*result_file) {
		perror(name);
		exit(1);
	}
}

void error_file()
{
	fprintf(stderr, "Error: invalid file\n");
	exit(1);
}

void generate_result_file(FILE *result_file, FILE *problem_file)
{
	char s[max_length_line], s_tmp[max_length_line], *start, *end, *there_is;
	int num;
	s[0] = '\0';
	start = s;
	while(strcmp(start, "end") != 0) {
		fputs(s, result_file);
		there_is = fgets(s, max_length_line, problem_file);
		if(!there_is)
			error_file();
		strcpy(s_tmp, s);
		end = s_tmp+strlen(s_tmp)-1;
		while(isspace(*end))
			end--;
		s_tmp[end-s_tmp+1] = '\0';
		start = s_tmp;
		while(isspace(*start))
			start++;
		if(strcmp(start, "begin") == 0) {
			fputs(s, result_file);
			fgets(s, max_length_line, problem_file);
			if(!there_is)
				error_file();
			start = s;
			while(isspace(*start))
				start++;
			num = strtol(start, &end, 10);
			if(start == end)
				error_file();
			sprintf(s, "%d 3 real\n", num+4);
		}
	}
	/*
	 * -offset_x+clip_x*100 + x >= 0
	 * offset_x+clip_x*100 - x >= 0 
	 * -offset_y+clip_y*100 + y >= 0
	 * offset_y+clip_y*100 - y >= 0 
	 */
	/*
	sprintf(s, "%lf 1 0\n", -offset_x+clip_x*1000);
	fputs(s, result_file);
	sprintf(s, "%lf -1 0\n", offset_x+clip_x*1000);
	fputs(s, result_file);
	sprintf(s, "%lf 0 1\n", -offset_y+clip_y*1000);
	fputs(s, result_file);
	sprintf(s, "%lf 0 -1\n", offset_y+clip_y*1000);
	fputs(s, result_file);
	*/
	fputs("10000 1 0\n", result_file);
	fputs("10000 -1 0\n", result_file);
	fputs("10000 0 1\n", result_file);
	fputs("10000 0 -1\n", result_file);
	fputs(s, result_file);
}

void get_file(FILE **f, char *file_name_out)
{
	FILE *problem_file, *result_file;
	char file_name_in[max_pathname_in];
	open_problem_file(&problem_file, file_name_in);
	open_result_file(&result_file, file_name_out);
	generate_result_file(result_file, problem_file);
	fclose(problem_file);
	fclose(result_file);
	*f = fopen(file_name_out, "r");
	if(!*f) {
		perror(file_name_out);
		exit(1);
	}
}

void get_cp_vertices(double **cp_vertices, unsigned int *cp_count_vertices)
{
  FILE *reading;
	char file_name[max_pathname_out];
  dd_ErrorType error=dd_NoError;
  dd_MatrixPtr M, G;

  dd_PolyhedraPtr poly;

  dd_set_global_constants(); /* First, this must be called once to use cddlib. */
  get_file(&reading, file_name);

/* Input an LP using the cdd library  */
  M = dd_PolyFile2Matrix(reading, &error);
  if (error!=dd_NoError) goto _L99;
  //dd_WriteMatrix(stdout, M);
/* Generate all vertices of the feasible reagion */
  poly = dd_DDMatrix2Poly(M, &error);
  G = dd_CopyGenerators(poly);
  //printf("rowsize: %ld\n", G->rowsize);
	order_vertices(cp_vertices, G->matrix, G->rowsize, G->colsize);
	*cp_count_vertices = G->rowsize;
  printf("\nGenerators (All the vertices of the feasible region if bounded.)\n");
  dd_WriteMatrix(stdout, G);

  /* Free allocated spaces. */
  dd_FreeMatrix(G);
  dd_FreePolyhedra(poly);

/* Free allocated spaces. */
  dd_FreeMatrix(M);
_L99:;
  fclose(reading);
#ifndef DONTDELETE
	unlink(file_name);
#endif
  if (error!=dd_NoError) dd_WriteErrorMessages(stdout, error);
  dd_free_global_constants();  /* At the end, this should be called. */
}

void prepare_convex_polygon(unsigned int *cp_VBO, unsigned int *cp_VAO,
		unsigned int cp_count_vertices, double *cp_vertices)
{
	glGenVertexArrays(1, cp_VAO);
	glGenBuffers(1, cp_VBO);
	glBindVertexArray(*cp_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, *cp_VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(double)*(cp_count_vertices)*6,
			cp_vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_DOUBLE, GL_FALSE, 6*sizeof(double),
			(void *)0);
	glEnableVertexAttribArray(0); //0 = Location in Vertex shader
	glVertexAttribPointer(1, 3, GL_DOUBLE, GL_FALSE, 6*sizeof(double),
			(void *)(3*sizeof(double)));
	glEnableVertexAttribArray(1); //1 = layout (Location) in Vertex shader
	//no needed VBO bind to GL_ARRAY_BUFFER now
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	//unbind VAO
	glBindVertexArray(0);
	free(cp_vertices);
}

void delete_convex_polygon(unsigned int cp_VBO, unsigned int cp_VAO)
{
	glDeleteVertexArrays(1, &cp_VAO);
	glDeleteBuffers(1, &cp_VBO);
}

int main()
{
	space *plane;
	unsigned int cp_VBO, cp_VAO, cp_count_vertices;
	double *cp_vertices;
	get_cp_vertices(&cp_vertices, &cp_count_vertices);
	/*
	unsigned int i;
	for(i = 0; i < *cp_count_vertices; i++) {
		printf("%lf;%lf;%lf;%lf;%lf;%lf\n", cp_vertices[i*6], cp_vertices[i*6+1],
				cp_vertices[i*6+2], cp_vertices[i*6+3], cp_vertices[i*6+4],
				cp_vertices[i*6+5]);
	}
	*/
	prepare_plane(&plane);
	prepare_convex_polygon(&cp_VBO, &cp_VAO, cp_count_vertices, cp_vertices);
	draw_graph(plane, draw_cp, cp_VAO, cp_count_vertices);
	delete_convex_polygon(cp_VBO, cp_VAO);
	delete_plane(plane);
	
	return 0;
}
