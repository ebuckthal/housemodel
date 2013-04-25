/*
 *  CPE 471 lab 4 - modern graphics test bed
 *  draws a partial cube using a VBO and IBO
 *  glut/OpenGL/GLSL application
 *  Uses glm and local matrix stack
 *  to handle matrix transforms for a view matrix, projection matrix and
 *  model transform matrix
 *
 *  zwood 9/12
 *  Copyright 2012 Cal Poly. All rights reserved.
 *
 *****************************************************************************/
#ifdef __APPLE__
#include "GLUT/glut.h"
#include <OPENGL/gl.h>
#endif
#ifdef __unix__
#include <GL/glut.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp" //perspective, trans etc
#include "glm/gtc/type_ptr.hpp" //value_ptr
#include "GLSL_helper.h"
#include "MStackHelp.h"

 using namespace std;
 using namespace glm;

//flag and ID to toggle on and off the shader
 int shade = 1;
 int ShadeProg;

//Handles to the shader data
 GLint h_aPosition;
 GLint h_uColor;
 GLint h_uModelMatrix;
 GLint h_uViewMatrix;
 GLint h_uProjMatrix;
 GLuint CubeBuffObj, CIndxBuffObj, GrndBuffObj, GIndxBuffObj, NormalBuffObj, RoofBuffObj, RIndxBuffObj;
 int g_CiboLen, g_GiboLen, g_RiboLen;

/* globals to control positioning and window size */
 static float g_width, g_height;
 float g_tx = 0;
 float g_ty = 0;
 float g_tz = 0;
 float g_basey = 0;
 float g_basex = 0;

//matrices
 mat4 CTM = mat4();
 mat4 TM = mat4();
 mat4 RM = mat4(1.0f);
 mat4 SM = mat4(1.0f);
 mat4 oRM = mat4(1.0f);

 float g_scale = 1.0;

/*highlight and non-highlight colors*/
 float g_highlight[] = {0.1, 0.2, 0.3};
 float g_color[] = {0.3, 0.3, 0.3};
 float *g_roof;
 float *g_cube;

 bool g_selRoof = false;
 bool g_selCube = false;
 bool g_selTranslate = false;
 bool g_selRotate = false;
 bool g_selScale = false;
 bool g_mousedown = false;

/*camera controls - do not change for Lab 5 */
 glm::vec3 g_trans(0, 0, -5.5);
 float g_angle = 0;

static const float g_groundY = -3.0;      // y coordinate of the ground
static const float g_groundSize = 10.0;   // half the ground length

//declare a matrix stack
RenderingHelper ModelTrans;

/* projection matrix - do not change - sets matrix in shader */
void SetProjectionMatrix() {
   glm::mat4 Projection = glm::perspective(80.0f, (float)g_width/g_height, 0.1f, 100.f);
   safe_glUniformMatrix4fv(h_uProjMatrix, glm::value_ptr(Projection));
}

/* camera controls - do not change - sets matrix in shader */
void SetView() {
   glm::mat4 Trans = glm::translate( glm::mat4(1.0f), g_trans);
   glm::mat4 RotateX = glm::rotate( Trans, g_angle, glm::vec3(0.0f, 1, 0));
   safe_glUniformMatrix4fv(h_uViewMatrix, glm::value_ptr(RotateX));
}

/* model transforms - do not change - sets matrix in shader */
void SetModel() {
   safe_glUniformMatrix4fv(h_uModelMatrix, glm::value_ptr(ModelTrans.modelViewMatrix));
}

/* intialize ground data */
static void initGround() {

   // A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
   float GrndPos[] = {
      -g_groundSize, g_groundY, -g_groundSize,
      -g_groundSize, g_groundY,  g_groundSize,
      g_groundSize, g_groundY,  g_groundSize,
      g_groundSize, g_groundY, -g_groundSize
   };
   unsigned short idx[] = {0, 1, 2, 0, 2, 3};

   g_GiboLen = 6;
   glGenBuffers(1, &GrndBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

   glGenBuffers(1, &GIndxBuffObj);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
}

static void initRoof() {


   float RoofPos[] = {
      -0.5, 0.0, -0.5,
      -0.5, 0.0, 0.5,
      0.5, 0.0, 0.5,
      0.5, 0.0, -0.5,
      0, 0.2, -0.5,
      0, 0.2, 0.5,
   };

   unsigned short idx[] = {
      0, 1, 4,
      5, 4, 1,
      2, 3, 4,
      5, 4, 2,
      0, 3, 4,
      1, 2, 5,
   };

   g_roof = g_color;
   g_RiboLen = 18;
   glGenBuffers(1, &RoofBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, RoofBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(RoofPos), RoofPos, GL_STATIC_DRAW);

   glGenBuffers(1, &RIndxBuffObj);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RIndxBuffObj);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
}

/* intialize the cube data */
static void initCube() {

   float CubePos[] = {
      -0.5, -0.5, -0.5, /*back face 5 verts :0 */
      -0.5, 0.5, -0.5,
      0.5, 0.5, -0.5,
      0.5, -0.5, -0.5,
      0.0, 0.5, -0.5,
      -0.5, -0.5, 0.5, /*front face 5 verts :5*/
      -0.5, 0.5, 0.5,
      0.5, 0.5, 0.5,
      0.5, -0.5, 0.5,
      0.0, 0.5, 0.5,
      -0.5, -0.5, 0.5, /*left face 4 verts :10*/
      -0.5, -0.5, -.5,
      -0.5, 0.5, -0.5,
      -0.5, 0.5, 0.5,
      0.5, -0.5, 0.5, /*right face 4 verts :14*/
      0.5, -0.5, -.5,
      0.5, 0.5, -0.5,
      0.5, 0.5, 0.5,
   };

   unsigned short idx[] = {
      0, 1, 2,
      2, 3, 0,
      1, 4, 2,
      5, 6, 7,
      7, 8, 5,
      6, 9, 7,
      10, 11, 12,
      12, 13, 10,
      14, 15, 16,
      16, 17, 14,
   };


   g_cube = g_color;
   g_CiboLen = 30;
   glGenBuffers(1, &CubeBuffObj);
   glBindBuffer(GL_ARRAY_BUFFER, CubeBuffObj);
   glBufferData(GL_ARRAY_BUFFER, sizeof(CubePos), CubePos, GL_STATIC_DRAW);

   glGenBuffers(1, &CIndxBuffObj);
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CIndxBuffObj);
   glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
}

void InitGeom() {
   initGround();
   initCube();
   initRoof();
}

/*function to help load the shaders (both vertex and fragment */
int InstallShader(const GLchar *vShaderName, const GLchar *fShaderName) {
   GLuint VS; //handles to shader object
   GLuint FS; //handles to frag shader object
   GLint vCompiled, fCompiled, linked; //status of shader

   VS = glCreateShader(GL_VERTEX_SHADER);
   FS = glCreateShader(GL_FRAGMENT_SHADER);

   //load the source
   glShaderSource(VS, 1, &vShaderName, NULL);
   glShaderSource(FS, 1, &fShaderName, NULL);

   //compile shader and print log
   glCompileShader(VS);
   /* check shader status requires helper functions */
   printOpenGLError();
   glGetShaderiv(VS, GL_COMPILE_STATUS, &vCompiled);
   printShaderInfoLog(VS);

   //compile shader and print log
   glCompileShader(FS);
   /* check shader status requires helper functions */
   printOpenGLError();
   glGetShaderiv(FS, GL_COMPILE_STATUS, &fCompiled);
   printShaderInfoLog(FS);

   if (!vCompiled || !fCompiled) {
      printf("Error compiling either shader %s or %s", vShaderName, fShaderName);
      return 0;
   }

   //create a program object and attach the compiled shader
   ShadeProg = glCreateProgram();
   glAttachShader(ShadeProg, VS);
   glAttachShader(ShadeProg, FS);

   glLinkProgram(ShadeProg);
   /* check shader status requires helper functions */
   printOpenGLError();
   glGetProgramiv(ShadeProg, GL_LINK_STATUS, &linked);
   printProgramInfoLog(ShadeProg);

   glUseProgram(ShadeProg);

   /* get handles to attribute data */
   h_aPosition = safe_glGetAttribLocation(ShadeProg, "aPosition");
   /* add a handle for the normal */
   h_uColor = safe_glGetUniformLocation(ShadeProg,  "uColor");
   h_uProjMatrix = safe_glGetUniformLocation(ShadeProg, "uProjMatrix");
   h_uViewMatrix = safe_glGetUniformLocation(ShadeProg, "uViewMatrix");
   h_uModelMatrix = safe_glGetUniformLocation(ShadeProg, "uModelMatrix");
   printf("sucessfully installed shader %d\n", ShadeProg);
   return 1;
}


/* Some OpenGL initialization */
void Initialize ()          // Any GL Init Code
{
   // Start Of User Initialization
   glClearColor (1.0f, 1.0f, 1.0f, 1.0f);
   // Black Background
   glClearDepth (1.0f);  // Depth Buffer Setup
   glDepthFunc (GL_LEQUAL);  // The Type Of Depth Testing
   glEnable (GL_DEPTH_TEST);// Enable Depth Testing

   /* some matrix stack init */
   ModelTrans.useModelViewMatrix();
   ModelTrans.loadIdentity();

}

/* Main display function */

void drawGround()
{
   /******************* set up to draw the ground plane */
   safe_glEnableVertexAttribArray(h_aPosition);
   // bind vbo
   glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
   safe_glVertexAttribPointer(h_aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
   // bind ibo
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);

   /* set the color in the shader */
   glUniform3f(h_uColor, 0.7, 0.98, 0.9);

   /* set the matrix stack to the identity - no transforms on the ground*/
   ModelTrans.loadIdentity();
   SetModel();
   // draw!
   glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

   // Disable the attributes used by our shader
   safe_glDisableVertexAttribArray(h_aPosition);
}

void drawCube()
{
   /*********************** set up to draw the cube plane */
   safe_glEnableVertexAttribArray(h_aPosition);
   // bind vbo
   glBindBuffer(GL_ARRAY_BUFFER, CubeBuffObj);
   safe_glVertexAttribPointer(h_aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
   // bind ibo
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CIndxBuffObj);

   //ModelTrans.pushMatrix();
   /* set up where to draw the box */


   //CTM = mat4(1.0f);
   CTM = SM * RM;
   glUniformMatrix4fv(h_uModelMatrix, 1, GL_FALSE, value_ptr(CTM));
   //SetModel();
   /* set the color in the shader */
   if(g_selCube) {
      g_cube = g_highlight;
   } else {
      g_cube = g_color;
   }
   glUniform3f(h_uColor, g_cube[0], g_cube[1], g_cube[2]);
   // draw!
   glDrawElements(GL_TRIANGLES, g_CiboLen, GL_UNSIGNED_SHORT, 0);
   //ModelTrans.popMatrix();
}


void drawRoof()
{
   /*********************** set up to draw the roof plane */
   safe_glEnableVertexAttribArray(h_aPosition);
   // bind vbo
   glBindBuffer(GL_ARRAY_BUFFER, RoofBuffObj);
   safe_glVertexAttribPointer(h_aPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
   // bind ibo
   glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, RIndxBuffObj);

   ModelTrans.pushMatrix();
   /* set up where to draw the box */
   //ModelTrans.translate(vec3(g_tx, g_ty+0.5, g_tz));
   //ModelTrans.scale(r*0.5, r*0.5, 1);
   SetModel();
   /* set the color in the shader */
   if(g_selRoof) {
      g_roof = g_highlight;
   } else {
      g_roof = g_color;
   }

   glUniform3f(h_uColor, g_roof[0], g_roof[1], g_roof[2]);
   // draw!
   glDrawElements(GL_TRIANGLES, g_RiboLen, GL_UNSIGNED_SHORT, 0);
   ModelTrans.popMatrix();
}

void Draw (void)
{

   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   //Start our shader
   glUseProgram(ShadeProg);

   /* only set the projection and view matrix once */
   SetProjectionMatrix();
   SetView();

   drawGround();
   drawRoof();
   drawCube();

   // Disable the attributes used by our shader
   safe_glDisableVertexAttribArray(h_aPosition);

   //Disable the shader
   glUseProgram(0);
   glutSwapBuffers();

}

/* Reshape */
void ReshapeGL (int width, int height)
{
   g_width = (float)width;
   g_height = (float)height;
   glViewport (0, 0, (GLsizei)(width), (GLsizei)(height));

}

//the keyboard callback to change the values to the transforms
void keyboard(unsigned char key, int x, int y ){
   switch( key ) {
      case '2':
      g_selRoof = !g_selRoof;
      g_selCube = false;
      break;
      case '1':
      g_selCube = !g_selCube;
      g_selRoof = false;
      break;
      case 't':
      g_selTranslate = !g_selTranslate;
      break;
      case 'r':
      g_selRotate = !g_selRotate;
      break;
      case 's':
      g_selScale = !g_selScale;
      break;
      case 'h':
      g_scale = 1.0;
      break;
      case 'q': case 'Q' :
      exit( EXIT_SUCCESS );
      break;
   }
   glutPostRedisplay();
}

vec3 getVec3(float xi, float yi)
{
   float vx, vy, vz;

   cout << xi << " " << yi << endl;

   if(sqrt((xi*xi)+(yi*yi)) <= 1)
   {
      vx = xi;
      vy = yi;
      vz = sqrt((1-(xi*xi)-(yi*yi)));
   }
   else
   {
      vx = xi / sqrt((xi*xi)+(yi*yi));
      vy = yi / sqrt((xi*xi)+(yi*yi));
      vz = 1;
   }

   cout << vx << " " << vy << " " << vz << endl;
   return vec3(vx, vy, vz);
}

void motion(int x, int y)
{
   int width = glutGet(GLUT_WINDOW_WIDTH);
   int height = glutGet(GLUT_WINDOW_HEIGHT);

   y = height - y;

   if(g_selScale)
   {
      g_scale = 5*abs((x-g_basex)/g_basex);
      cout << g_scale << endl;
      SM = scale(mat4(1.0f), vec3(g_scale));

   }

   if(g_selTranslate)
   {
   }

   if(g_selRotate)
   {
      float xi = x/(width/2.0) - 1;
      float yi = y/(height/2.0) - 1;
      
      float xi_base = g_basex/(width/2.0) - 1;
      float yi_base = g_basey/(height/2.0) - 1;
      
      vec3 now = getVec3(xi, yi);
      vec3 start = getVec3(xi_base, yi_base);

      float angle = acos(dot(now,start)) * (180.0/3.14159);
      vec3 n = cross(start, now);
      
      RM = rotate(mat4(1.0f), angle, n); 
   }




   glutPostRedisplay();
}

void mouse(int button, int state, int x, int y)
{
   if(state == 0) //down
   {
      g_mousedown = true;
      g_basex = x;
      g_basey = y;
   }
   else if(state == 1) //up
   {
      g_mousedown = false;
   }
}

int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 20, 20 );
   glutInitWindowSize( 1000, 1000 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow("Cube and Transforms");
   glutReshapeFunc( ReshapeGL );
   glutDisplayFunc( Draw );
   glutKeyboardFunc( keyboard );
   glutMouseFunc( mouse );
   glutMotionFunc ( motion );
   Initialize();

   //test the openGL version
   getGLversion();
   //install the shader
   if (!InstallShader(textFileRead((char *)"Lab5_vert.glsl"), textFileRead((char *)"Lab5_frag.glsl"))) {
      printf("Error installing shader!\n");
      return 0;
   }

   InitGeom();
   glutMainLoop();
   return 0;
}
