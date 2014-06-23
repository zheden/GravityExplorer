﻿
#define GLFW_INCLUDE_GLU
#define _USE_MATH_DEFINES

#include <windows.h>
#include <glfw3.h>
#include <GL/glut.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "ImgLoader.h"
#include "PoseEstimation.h"
#include "MarkerTracker.h"
#include "Tvector.h"
#include "MatrixInverse.h"

#include "globals.h"
#include "SpaceObjects.h"

#include <iostream>
using namespace std;

//////////////////////////////////////////////////////////////////////////
void InitObjects()
{
   planets.clear();
   satellites.clear();

   Image* img = loadBMP("graphics//planet1.bmp");
   const GLuint textureId0 = loadTextureFromImage(img);
   delete img;

   Image* img1 = loadBMP("graphics//planet2.bmp");
   const GLuint textureId1 = loadTextureFromImage(img1);
   delete img1;

   gp_quadratic = gluNewQuadric();
   gluQuadricDrawStyle(gp_quadratic, GLU_FILL);
   gluQuadricTexture(gp_quadratic, GL_TRUE);
   gluQuadricNormals(gp_quadratic, GLU_SMOOTH);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // earth
   const TVector earth_pos = TVector(0, 0, 0);
   StaticPlanet earth(earth_pos, 500000000, textureId0, 0x005a);
   planets.push_back(earth);

   // saturn
   const TVector saturn_pos = TVector(0, 0, 0);
   StaticPlanet saturn(saturn_pos, 500000000, textureId1, 0x0b44);
   planets.push_back(saturn);

   const TVector moon_velo = TVector(-1, 0, 0);
   const TVector moon_pos = TVector(0, 270, 0);
   Satellite moon(moon_velo, moon_pos, earth.m_mass / 100 /*g_aster_model.get()*/);
   satellites.push_back(moon);

   g_last_time = glfwGetTime();
}

//////////////////////////////////////////////////////////////////////////
void OnKeyPressed(GLFWwindow* window, int i_key, int scancode, int i_action, int mods)
{
   if (i_action != GLFW_PRESS)
   {
      return;
   }

   switch (i_key)
   {
   case GLFW_KEY_SPACE:
      g_is_spin_mode = !g_is_spin_mode;
      break;
   case GLFW_KEY_UP:
      if (glfwGetKey(window, GLFW_KEY_M))
      {
         // m + up

         // increment mass and radius of main planet
         if (planets.empty())
            return;

         planets[0].m_mass *= 1.3;
         planets[0].m_radius_scale *= 1.05;
         break;
      }
      else if (glfwGetKey(window, GLFW_KEY_V))
      {
         // v + up
         // increment velocity of main satellite
         if (satellites.empty())
            return;

         satellites[0].m_velocity *= 1.1;
         break;
      }
      else if (glfwGetKey(window, GLFW_KEY_X))
      {
         rotateAroundXaxis(0.25);
         break;
      }
      else if (glfwGetKey(window, GLFW_KEY_Y))
      {
         rotateAroundYaxis(0.25);
         break;
      }
      else if (glfwGetKey(window, GLFW_KEY_Z))
      {
         rotateAroundZaxis(0.25);
         break;
      }
      g_animate_increment *= 2.0;
      break;
   case GLFW_KEY_DOWN:
      if (glfwGetKey(window, GLFW_KEY_M))
      {
         // m + down
         // decrement mass and radius of main planet.
         if (planets.empty())
            return;

         planets[0].m_mass /= 1.3;
         planets[0].m_radius_scale /= 1.05;
         break;
      }
      else if (glfwGetKey(window, GLFW_KEY_V))
      {
         // v + down 
         // decrement velocity of main satellite
         if (satellites.empty())
            return;

         satellites[0].m_velocity *= 10.0/11.0; // /=1.1
         break;
      }
      else if (glfwGetKey(window, GLFW_KEY_X))
      {
         rotateAroundXaxis(-0.25);
         break;
      }
      else if (glfwGetKey(window, GLFW_KEY_Y))
      {
         rotateAroundYaxis(-0.25);
         break;
      }
      else if (glfwGetKey(window, GLFW_KEY_Z))
      {
         rotateAroundZaxis(-0.25);
         break;
      }
      g_animate_increment /= 2.0;
      break;
   case GLFW_KEY_R:
      InitObjects();
      break;
   case GLFW_KEY_ESCAPE:
      exit(1);
   }
}

//////////////////////////////////////////////////////////////////////////
void UpdateState()
{
   double time = glfwGetTime();
   double time_interval = time - g_last_time;
   time_interval *= g_animate_increment;
   g_last_time = time;

   if (!g_is_spin_mode)
      return; // nothing to update

   for (int si = 0; si < satellites.size(); ++si)
   {
      TVector acceleration_vec(0, 0, 0);
      for (int pi = 0; pi < planets.size(); ++pi)
      {
         if ( !planets[pi].m_is_on_scene)
            continue;

         TVector distance_vec; ///? remove from global
         if (pi == 0) ///? bbee everything is connected to coor sys 0. change!!
         {
            distance_vec = planets[pi].m_pos - satellites[si].m_pos;
         }
         else
         {
            CMatrix mat_to("mat1", 4, 4);
            mat_to.SetData(planets[0].m_resultMatrix);
            CMatrix mat_to_inv = mat_to.Inverse();

            CMatrix mat_from("mat1", 4, 4);
            mat_from.SetData(planets[pi].m_resultMatrix);
            CMatrix T = mat_to_inv * mat_from;

            CMatrix center("cent", 4, 1);
            center.m_pData[0][0] = 0;
            center.m_pData[1][0] = 0;
            center.m_pData[2][0] = 0;
            center.m_pData[3][0] = 1;

            CMatrix res = T * center;

            distance_vec = TVector(res.m_pData[0][0], res.m_pData[1][0], res.m_pData[2][0]);
            distance_vec *= 1/g_scale_factor_for_draw;

            distance_vec -= satellites[si].m_pos;

            distance_vec_draw = distance_vec;


         }
         const double dist_moon_earth = distance_vec.length();
         const TVector gravity_direction = distance_vec.normalize();

         // ma = M*m*G/r^2 ///? o o not correct
         const double acceleration_scal = G * planets[pi].m_mass / (dist_moon_earth * dist_moon_earth);
         // * g_scale_factor only distances are huge, velocity and acceleration are small ///? change this, make numbers as in real life

         acceleration_vec += gravity_direction * acceleration_scal;
      }
      satellites[si].acceleration_vec = acceleration_vec;
      // calculate position of moon according to its velocity and position of earth
      satellites[si].m_velocity += acceleration_vec * time_interval;
      satellites[si].m_pos += satellites[si].m_velocity * time_interval;
   }

   g_hour_of_day += g_animate_increment;

   g_hour_of_day = g_hour_of_day - ((int)(g_hour_of_day/g_num_hours_in_day))*g_num_hours_in_day;
}

//////////////////////////////////////////////////////////////////////////
void initVideoStream( cv::VideoCapture &cap ) {
   if( cap.isOpened() )
      cap.release();

   cap.open(0); // open the default camera
   if ( cap.isOpened()==false ) {
      std::cout << "No webcam found, using a video file" << std::endl;
      cap.open("MarkerMovie.mpg");
      if ( cap.isOpened()==false ) {
         std::cout << "No video file found. Exiting."      << std::endl;
         exit(0);
      }
   }

}

//////////////////////////////////////////////////////////////////////////
void InitGL(int argc, char *argv[])
{
   // pixel storage/packing stuff
   glPixelStorei( GL_PACK_ALIGNMENT,   1 ); // for glReadPixels​
   glPixelStorei( GL_UNPACK_ALIGNMENT, 1 ); // for glTexImage2D​
   glPixelZoom( 1.0, -1.0 );

   glShadeModel(GL_SMOOTH);
   glDisable(GL_CULL_FACE);
   glEnable(GL_TEXTURE_2D);

   // enable and set colors
   glEnable( GL_COLOR_MATERIAL );
   glClearColor( 0, 0.1, 0.1, 1.0 );

   // enable and set depth parameters
   glEnable( GL_DEPTH_TEST );
   glClearDepth( 1.0 );

   // light parameters
   GLfloat light_pos[] = { 1.0, 1.0, 1.0, 0.0 };
   GLfloat light_amb[] = { 0.2, 0.2, 0.2, 1.0 };
   GLfloat light_dif[] = { 0.7, 0.7, 0.7, 1.0 };

   // enable lighting
   glLightfv( GL_LIGHT0, GL_POSITION, light_pos );
   glLightfv( GL_LIGHT0, GL_AMBIENT,  light_amb );
   glLightfv( GL_LIGHT0, GL_DIFFUSE,  light_dif );
   glEnable( GL_LIGHTING );
   glEnable( GL_LIGHT0 );
}

//////////////////////////////////////////////////////////////////////////
void Reshape( GLFWwindow* window, int width, int height ) {

   // set a whole-window viewport
   glViewport( 0, 0, (GLsizei)width, (GLsizei)height );

   // create a perspective projection matrix
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   // Note: Just setting the Perspective is an easy hack. In fact, the camera should be calibrated.
   // With such a calibration we would get the projection matrix. This matrix could then be loaded 
   // to GL_PROJECTION.
   // If you are using another camera (which you'll do in most cases), you'll have to adjust the FOV
   // value. How? Fiddle around: Move Marker to edge of display and check if you have to increase or 
   // decrease.
   gluPerspective( g_virtual_camera_angle, ((GLfloat)width/(GLfloat)height), 0.01, 100 );

   // invalidate display
   //glutPostRedisplay();
}

//////////////////////////////////////////////////////////////////////////
void Display( GLFWwindow* window, const cv::Mat &img_bgr, std::vector<Marker> &markers) 
{
   memcpy( bkgnd, img_bgr.data, sizeof(bkgnd) );

   // clear buffers
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   // move to origin
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // draw background image
   glDisable( GL_DEPTH_TEST );

   glMatrixMode( GL_PROJECTION );
   glPushMatrix();
   glLoadIdentity();
   gluOrtho2D( 0.0, g_camera_width, 0.0, g_camera_height );

   glRasterPos2i( 0, g_camera_height-1 );
   glDrawPixels( g_camera_width, g_camera_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, bkgnd );

   glPopMatrix();

   glEnable(GL_DEPTH_TEST);

   // move to marker-position
   glMatrixMode( GL_MODELVIEW );

   if (markers.empty())
      return;


   for (int k = 0; k < planets.size(); ++k)
      planets[k].m_is_on_scene = false;

   for(int i=0; i<markers.size(); i++)
   {
      const int code = markers[i].code;
      for (int k = 0; k < planets.size(); ++k)
      {
         if(code == planets[k].m_code)
         {
            planets[k].m_is_on_scene = true;
            for(int j=0; j<16; j++)
               planets[k].m_resultMatrix[j] = markers[i].resultMatrix[j];
         }
      }
   }


   for (int i = 0; i < planets.size(); ++i)
   {
      if (!planets[i].m_is_on_scene)
         continue;

      //glPushMatrix();
      float resultTransposedMatrix[16];
      for (int x=0; x<4; ++x)
         for (int y=0; y<4; ++y)
            resultTransposedMatrix[x*4+y] = planets[i].m_resultMatrix[y*4+x];

      glLoadMatrixf( resultTransposedMatrix );
      //glRotatef( -90, 1, 0, 0 );

      glScalef(g_scale_factor_for_draw, g_scale_factor_for_draw, g_scale_factor_for_draw);

      glPushMatrix();
      glRotatef(360.0 * g_hour_of_day / g_num_hours_in_day, 1.0, 0.0, 0.0); // to make slower rotation
      glScaled(planets[i].m_radius_scale, planets[i].m_radius_scale, planets[i].m_radius_scale);
      //glColor3f(0.2, 0.2, 1.0);
      //glutWireSphere(90, 15, 15);


      glBindTexture(GL_TEXTURE_2D, planets[i].m_texID);

      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

      gluSphere(gp_quadratic, 100.0, 20, 20);

      glBindTexture(GL_TEXTURE_2D, NULL);

      glPopMatrix();


      //if (i == 0)
      //{
      //   glTranslatef(satellites[0].m_pos.X(), satellites[0].m_pos.Y(), satellites[0].m_pos.Z());
      //   glBegin(GL_LINES);
      //   glVertex3f(0, 0, 0);
      //   glVertex3f(distance_vec_draw.X(), distance_vec_draw.Y(), distance_vec_draw.Z());
      //   glEnd();
      //}
   }

   for (int i = 0; i < satellites.size(); ++i)
   {
      if (!planets[0].m_is_on_scene)
         break;
      //same transform as first planet
      float resultTransposedMatrix[16];
      for (int x=0; x<4; ++x)
         for (int y=0; y<4; ++y)
            resultTransposedMatrix[x*4+y] = planets[0].m_resultMatrix[y*4+x];

      glLoadMatrixf( resultTransposedMatrix );
      //glRotatef( -90, 1, 0, 0 );
      glScalef(0.0003, 0.0003, 0.0003);

      // day_of_year determine rotation around the earth
      //glRotatef(360.0 * g_day_of_month / 30.0, 0.0, 1.0, 0.0);
      glTranslatef(satellites[i].m_pos.X(), satellites[i].m_pos.Y(), satellites[i].m_pos.Z());

      glEnable(GL_COLOR_MATERIAL);
      glColor3f(0, 0, 1);
      glBegin(GL_LINES);
      glVertex3f(0, 0, 0);
      glVertex3f(satellites[i].m_velocity.X() * 100, 
         satellites[i].m_velocity.Y() * 100, 
         satellites[i].m_velocity.Z() * 100);
      glEnd();

      const int scale_for_draw = 10000;
      glEnable(GL_COLOR_MATERIAL);
      glColor3f(1, 0, 0);
      glBegin(GL_LINES);
      glVertex3f(0, 0, 0);
      glVertex3f(satellites[i].acceleration_vec.X() * scale_for_draw, 
         satellites[i].acceleration_vec.Y() * scale_for_draw, 
         satellites[i].acceleration_vec.Z() * scale_for_draw);
      glEnd();


      glColor3f(1, 0.2, 1.0);
      glutWireSphere(30, 20, 20);
      //glDisable(GL_COLOR_MATERIAL);
      //Lib3dsFile * p_file = satellites[i].m_3d_model_file;
      //render_node(p_file->nodes, p_file);
      //glEnable(GL_COLOR_MATERIAL);
   }

   // for debug: draw line betw 2 planets
   //bool show_dist = false;

   //glLoadIdentity();
   //glTranslatef(planets[0].m_resultMatrix[3], planets[0].m_resultMatrix[7], planets[0].m_resultMatrix[11]);
   //if (show_dist && planets.size() == 2)
   //{
   //   //glEnable(GL_COLOR_MATERIAL);
   //   glColor3f(1, 1, 1);
   //   glBegin(GL_LINES);
   //   //glVertex3f(planets[0].m_resultMatrix[3], planets[0].m_resultMatrix[7], planets[0].m_resultMatrix[11]);
   //   glVertex3f(0,0,0);
   //   glVertex3f(planets[1].m_resultMatrix[3]*100, planets[1].m_resultMatrix[7]*100, planets[1].m_resultMatrix[11]*100);
   //   glEnd();
   //}
}

//////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) 
{
   GLFWwindow* window;

   if (!glfwInit())
      return -1;

   // initialize the window system
   /* Create a windowed mode window and its OpenGL context */
   window = glfwCreateWindow(g_camera_width, g_camera_height, "Combine", NULL, NULL);
   if (!window)
   {
      glfwTerminate();
      return -1;
   }

   glfwSetFramebufferSizeCallback(window, Reshape);

   glfwMakeContextCurrent(window);
   glfwSwapInterval( 1 );

   int window_width, window_height;
   glfwGetFramebufferSize(window, &window_width, &window_height);
   Reshape(window, window_width, window_height);

   glViewport(0, 0, window_width, window_height);

   InitGL(argc, argv);
   InitObjects();

   glfwSetKeyCallback(window, OnKeyPressed);

   // setup OpenCV
   cv::Mat img_bgr;
   initVideoStream(cap);
   const double kMarkerSize = 0.048; // [m]
   MarkerTracker markerTracker(kMarkerSize);

   std::vector<Marker> markers;
   while (!glfwWindowShouldClose(window))
   {
      cap >> img_bgr;

      if(img_bgr.empty())
      {
         std::cout << "Could not query frame. Trying to reinitialize." << std::endl;
         initVideoStream(cap);
         cv::waitKey(1000);
         continue;
      }
      markers.clear();
      markerTracker.findMarker( img_bgr, markers);
      Display(window,           img_bgr, markers);
      UpdateState();

      glfwSwapBuffers(window);
      glfwPollEvents();
   }

   gluDeleteQuadric(gp_quadratic);
   glfwTerminate();
   return 0;
}