﻿
#define GLFW_INCLUDE_GLU
#define _USE_MATH_DEFINES

#include <windows.h>
#include <GL/glew.h>
#include <glfw3.h>
#include <GL/glut.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include <iostream>
#include <list>

#include "ImgLoader.h"
#include "PoseEstimation.h"
#include "MarkerTracker.h"
#include "Tvector.h"
#include "MatrixInverse.h"

#include "globals.h"
#include "SpaceObjects.h"
#include "ParticleManager.h"

void InitTextures()
{
   Image* img = loadBMP("graphics//planet1.bmp");
   g_textureId0 = loadTextureFromImage(img);
   delete img;

   Image* img1 = loadBMP("graphics//planet2.bmp");
   g_textureId1 = loadTextureFromImage(img1);
   delete img1;

   Image* img2 = loadBMP("graphics//planet4.bmp");
   g_textureId2 = loadTextureFromImage(img2);
   delete img2;

   Image* img_ast = loadBMP("graphics//asteroid.bmp");
   g_textureId_ast = loadTextureFromImage(img_ast);
   delete img_ast;

   gp_quadratic = gluNewQuadric();
   gluQuadricDrawStyle(gp_quadratic, GLU_FILL);
   gluQuadricTexture(gp_quadratic, GL_TRUE);
   gluQuadricNormals(gp_quadratic, GLU_SMOOTH);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glBindTexture(GL_TEXTURE_2D, NULL);
}

using namespace std;

//////////////////////////////////////////////////////////////////////////
void InitObjects()
{
   planets.clear();
   satellites.clear();
   particles.clear();

   // earth
   const TVector earth_pos = TVector(0, 0, 0);
   StaticPlanet earth(earth_pos, 45, g_textureId0, 0x005a);
   planets.push_back(earth);

   // saturn
   const TVector saturn_pos = TVector(0, 0, 0);
   StaticPlanet saturn(saturn_pos, 45, g_textureId1, 0x0b44);
   planets.push_back(saturn);

   StaticPlanet black_planet(TVector(0, 0, 0), 70, g_textureId2, 0x0690);
   black_planet.m_radius_scale = 1.3;
   planets.push_back(black_planet);

   const TVector satell_velo = TVector(-0.02, 0, 0);
   const TVector satell_pos = TVector(0, 0, 0);
   Satellite satell(satell_velo, satell_pos, earth.m_mass / 100, g_textureId_ast, 0x1228);
   satellites.push_back(satell);

   g_last_time = glfwGetTime();
}

//////////////////////////////////////////////////////////////////////////
void Reset()
{
   g_sat_pos_is_calculated = false;
   InitObjects();
   g_is_spin_mode = false;
   g_animate_increment = 2;

   g_is_pending_reset = false;
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
      g_initialization_is_done = true;
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
      if (g_animate_increment < 8)
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
      Reset();
      g_initialization_is_done = false;
      break;
   case GLFW_KEY_ESCAPE:
      exit(1);
   }
}

//////////////////////////////////////////////////////////////////////////
TVector GetPointInAnotherCoorSys(const TVector& i_point_from, float i_mat_from[16], float i_mat_to[16])
{
   CMatrix mat_to("mat1", 4, 4);
   mat_to.SetData(i_mat_to);
   CMatrix mat_to_inv = mat_to.Inverse();

   CMatrix mat_from("mat1", 4, 4);
   mat_from.SetData(i_mat_from);
   CMatrix T = mat_to_inv * mat_from;

   CMatrix point_from("cent", 4, 1);
   point_from.m_pData[0][0] = i_point_from.X();
   point_from.m_pData[1][0] = i_point_from.Y();
   point_from.m_pData[2][0] = i_point_from.Z();
   point_from.m_pData[3][0] = 1;

   CMatrix point_to = T * point_from;
   const double homo_coordinate = 1; ///? point_to.m_pData[3][0];

   const double x = point_to.m_pData[0][0] / homo_coordinate;
   const double y = point_to.m_pData[1][0] / homo_coordinate;
   const double z = point_to.m_pData[2][0] / homo_coordinate;

   return TVector(x, y, z);
}

void CopyMatrixLerp(float from[16], float to[16], float damping)
{
	for(int j=0; j<16; j++)
	{
		// If the difference is too big we don't interpolate
		if (abs(from[j] - to[j]) > 1000.0)
		{
			to[j] = from[j];
		}
		else
		{
			to[j] += (from[j] - to[j]) * damping;
		}
	}
}

//////////////////////////////////////////////////////////////////////////
void UpdateState(std::vector<Marker> &markers)
{
   const double time = glfwGetTime();
   double delta_time = time - g_last_time;
   delta_time *= g_animate_increment;
   g_last_time = time;

   if (!g_initialization_is_done)
      Reset();

   if (g_is_pending_reset)
   {
      g_time_until_reset -= delta_time;
      if (g_time_until_reset <= 0)
      {
         Reset();
         g_initialization_is_done = false; 
      }
   }

   const double fixed_time_step = 0.07 * g_animate_increment;

   if (!markers.empty())
   {
      //////////////////////////////////////////////////////////////////////////
      // determine what planets and satellites will be shown
      for (uint k = 0; k < planets.size(); ++k)
         planets[k].m_is_on_scene = false;
      for (uint k = 0; k < satellites.size(); ++k)
         satellites[k].m_is_on_scene = false;

      for(uint i=0; i<markers.size(); i++)
      {
         const int code = markers[i].code;
         bool marker_i_found = false;
         for (uint k = 0; k < planets.size() && !marker_i_found; ++k)
         {
            if(code == planets[k].m_code)
            {
               planets[k].m_is_on_scene = true;
			   CopyMatrixLerp(markers[i].resultMatrix, planets[k].m_resultMatrix, 0.9);
               marker_i_found = true;
            }
         }

         for (uint k = 0; k < satellites.size() && !marker_i_found; ++k)
         {
            if(code == satellites[k].m_code)
            {
			   satellites[k].m_is_on_scene = true;
			   CopyMatrixLerp(markers[i].resultMatrix, satellites[k].m_resultMatrix, 0.75);
               marker_i_found = true;
            }
         }
      }
   }

   if (!g_sat_pos_is_calculated)
   {
      if (!planets[0].m_is_on_scene || !satellites[0].m_is_on_scene)
         return;

      // pos of satellite now is zero
      TVector distance_vec_sat_planet = GetPointInAnotherCoorSys(TVector(0, 0, 0), planets[0].m_resultMatrix, satellites[0].m_resultMatrix); // point is dist because planet is in zero

      const double length = distance_vec_sat_planet.length();
      const double part = 0.068 / length;
      satellites[0].m_pos += distance_vec_sat_planet - distance_vec_sat_planet * part;

      //TVector velo_sat = GetPointInAnotherCoorSys(TVector(0, -1, 0), planets[0].m_resultMatrix, satellites[0].m_resultMatrix);
      //satellites[0].m_velocity = velo_sat * satellites[0].m_velocity.length();

      g_sat_pos_is_calculated = true;
   }

   if (!g_is_spin_mode)
      return; // nothing to update

   if (!g_is_pending_reset)
   {
	   for (uint si = 0; si < satellites.size(); ++si)
	   {
		   TVector acceleration_vec(0, 0, 0);
		   bool found_close_planet = true;

		   for (uint pi = 0; pi < planets.size(); ++pi)
		   {
			   if ( !planets[pi].m_is_on_scene)
				   continue;

			   TVector distance_vec_sat_planet = GetPointInAnotherCoorSys(TVector(0, 0, 0), planets[pi].m_resultMatrix, satellites[si].m_resultMatrix); // point is dist because planet is in zero
			   distance_vec_sat_planet -= satellites[si].m_pos;

			   distance_vec_draw = distance_vec_sat_planet; // only for debug

			   const double dist_sat_planet = distance_vec_sat_planet.length();

			   // Check for collision between sat and planet
			   if (dist_sat_planet < 0.03 * planets[pi].m_radius_scale + 0.005)
			   {
				   // Create explosion particle effect
				   float pos[3] = {-satellites[si].m_velocity.X() * fixed_time_step, -satellites[si].m_velocity.Y() * fixed_time_step, -satellites[si].m_velocity.Z() * fixed_time_step};
				   AddParticles(pos, 200, PARTICLE_FLYING, 0.2);
				   AddParticles(pos, 100, PARTICLE_STRETCHING, 0.1);

				   // Schedule reset
				   g_is_pending_reset = true;
				   g_time_until_reset = g_reset_timeout;
			   }

			   // Check if the planet is close (to reset if the satellite is far from all planets)
			   if (dist_sat_planet >= 0.3)
			   {
				   found_close_planet = false;
			   }

			   const TVector gravity_direction = distance_vec_sat_planet.normalize();

			   // ma = M*m*G/r^2
			   const double acceleration_scal = G * planets[pi].m_mass / (dist_sat_planet * dist_sat_planet);

			   acceleration_vec += gravity_direction * acceleration_scal;
		   }

		   satellites[si].m_acceleration_vec = acceleration_vec;
		   // calculate position of moon according to its velocity and position of earth
      satellites[si].m_velocity += acceleration_vec * fixed_time_step;

      if (satellites[si].m_tail.empty() || satellites[si].m_tail.front().dist(satellites[si].m_pos) > 0.001)
         satellites[si].m_tail.push_front(satellites[si].m_pos); // push old pos
      while (satellites[si].m_tail.size() > 50)
         satellites[si].m_tail.pop_back();
      satellites[si].m_pos += satellites[si].m_velocity * fixed_time_step;

		   // Reset if the satellite is too far from all planets
		   if (!found_close_planet)
		   {
			   // Schedule reset
			   g_is_pending_reset = true;
			   g_time_until_reset = g_reset_timeout;
		   }
	   }
   }
   
   // needed for rotation of planet around its axis
   g_hour_of_day += g_animate_increment;
   g_hour_of_day = g_hour_of_day - ((int)(g_hour_of_day/g_num_hours_in_day))*g_num_hours_in_day;

   updateParticles(fixed_time_step);
}

//////////////////////////////////////////////////////////////////////////
void initVideoStream( cv::VideoCapture &cap ) {
   if( cap.isOpened() )
      cap.release();

   cap.open(0); // open the default camera
   if ( cap.isOpened()==false ) {
      std::cout << "No webcam found, using a video file" << std::endl;
      //cap.open("MarkerMovie.mpg");
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

   const GLfloat scale_width = ((GLfloat)g_initial_window_width) / g_camera_width;
   const GLfloat scale_height = ((GLfloat)g_initial_window_height) / g_camera_height;
   glPixelZoom(scale_width, -scale_height);

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
void Reshape( GLFWwindow* window, int width, int height )
{
   const GLfloat scale_width = ((GLfloat)width) / g_camera_width;
   const GLfloat scale_height = ((GLfloat)height) / g_camera_height;
   glPixelZoom(scale_width, -scale_height);

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
void DrawSatelliteTail(uint index)
{
   glEnable(GL_BLEND);
   glDisable( GL_LIGHTING );
   glDisable( GL_LIGHT0 );
   glColor4f(1, 1, 0, 0.3);
   glPushMatrix();
   const std::list<TVector> tail = satellites[index].m_tail;

   GLfloat scale_factor = 1;
   for (std::list<TVector>::const_iterator it=tail.begin(); it != tail.end(); ++it)
   {
      const TVector pos = *it;
      glPushMatrix();
      glTranslatef(pos.X(), pos.Y(), pos.Z());
      glScalef(scale_factor, scale_factor, scale_factor);
      glColor4f(1, 1, 0, scale_factor * 0.3);
      //gluSphere(gp_quadratic, 0.004, int(scale_factor * 10), int(scale_factor * 10));
      glutWireSphere(0.004, int(scale_factor * 10), int(scale_factor * 10));
      glPopMatrix();

      scale_factor *= 0.98;
   }
   glPopMatrix();
   glDisable(GL_BLEND);
   glEnable( GL_LIGHTING );
   glEnable( GL_LIGHT0 );
}

//////////////////////////////////////////////////////////////////////////
void DrawSatellite(uint index)
{
   GLuint i_texID = satellites[index].m_texID;
   glPushMatrix();
   glColor3f(1, 1, 1);
   glScalef(0.0003, 0.0003, 0.0003); // scale just drawing of planet
   glScalef(2, 2, 2);
   glBindTexture(GL_TEXTURE_2D, i_texID);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   gluSphere(gp_quadratic, 10.0, 15, 15);
   glBindTexture(GL_TEXTURE_2D, NULL);

   glPushMatrix();
   glTranslatef(4, 4, 4);
   glBindTexture(GL_TEXTURE_2D, i_texID);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   gluSphere(gp_quadratic, 9.0, 10, 10);
   glBindTexture(GL_TEXTURE_2D, NULL);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(-5, 0, 5);
   glBindTexture(GL_TEXTURE_2D, i_texID);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   gluSphere(gp_quadratic, 6, 10, 10);
   glBindTexture(GL_TEXTURE_2D, NULL);
   glPopMatrix();

   glPushMatrix();
   glTranslatef(0, -5, -3);
   glBindTexture(GL_TEXTURE_2D, i_texID);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   gluSphere(gp_quadratic, 6, 10, 10);
   glBindTexture(GL_TEXTURE_2D, NULL);
   glPopMatrix();

   glPopMatrix();
}

//////////////////////////////////////////////////////////////////////////
void DrawPlanet(uint index)
{
   glPushMatrix();
   glRotatef(360.0 * g_hour_of_day / g_num_hours_in_day, 0.0, 0.0, 1.0); // to make slower rotation
   glScaled(planets[index].m_radius_scale, planets[index].m_radius_scale, planets[index].m_radius_scale);

   //glColor3f(0.2, 0.2, 1.0);
   //glutWireSphere(40, 15, 15);

   glBindTexture(GL_TEXTURE_2D, planets[index].m_texID);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glColor3f(1, 1, 1);
   gluSphere(gp_quadratic, 0.03, 20, 20);
   glBindTexture(GL_TEXTURE_2D, NULL);

   glPopMatrix();
}

#define RADPERDEG 0.0174533

void DrawArrow(GLdouble x1,GLdouble y1,GLdouble z1,GLdouble x2,GLdouble y2,GLdouble z2,GLdouble D)
{
   double x=x2-x1;
   double y=y2-y1;
   double z=z2-z1;
   double L=sqrt(x*x+y*y+z*z);

   GLUquadricObj *quadObj;

   glPushMatrix ();

   glTranslated(x1,y1,z1);

   if((x!=0.)||(y!=0.)) {
      glRotated(atan2(y,x)/RADPERDEG,0.,0.,1.);
      glRotated(atan2(sqrt(x*x+y*y),z)/RADPERDEG,0.,1.,0.);
   } else if (z<0){
      glRotated(180,1.,0.,0.);
   }

   glTranslatef(0,0,L-4*D);

   quadObj = gluNewQuadric ();
   gluQuadricDrawStyle (quadObj, GLU_FILL);
   gluQuadricNormals (quadObj, GLU_SMOOTH);
   gluCylinder(quadObj, 2*D, 0.0, 4*D, 32, 1);
   gluDeleteQuadric(quadObj);

   quadObj = gluNewQuadric ();
   gluQuadricDrawStyle (quadObj, GLU_FILL);
   gluQuadricNormals (quadObj, GLU_SMOOTH);
   gluDisk(quadObj, 0.0, 2*D, 32, 1);
   gluDeleteQuadric(quadObj);

   glTranslatef(0,0,-L+4*D);

   quadObj = gluNewQuadric ();
   gluQuadricDrawStyle (quadObj, GLU_FILL);
   gluQuadricNormals (quadObj, GLU_SMOOTH);
   gluCylinder(quadObj, D, D, L-4*D, 32, 1);
   gluDeleteQuadric(quadObj);

   quadObj = gluNewQuadric ();
   gluQuadricDrawStyle (quadObj, GLU_FILL);
   gluQuadricNormals (quadObj, GLU_SMOOTH);
   gluDisk(quadObj, 0.0, D, 32, 1);
   gluDeleteQuadric(quadObj);

   glPopMatrix();
   }

void DisplatText(const std::string& i_str)
{
   glPushMatrix();
   glLoadIdentity();
   gluOrtho2D( -700, g_camera_width*4, -g_camera_height + 50, g_camera_height*4 );

   glColor3f(1, 0, 0);
   glutStrokeWidth(GLUT_STROKE_ROMAN, 120);

   for (auto c = i_str.begin(); c != i_str.end(); ++c)
   {
      glutStrokeCharacter(GLUT_STROKE_ROMAN, *c);
   }
   glPopMatrix();
}


//////////////////////////////////////////////////////////////////////////
void Display( GLFWwindow* window, const cv::Mat &img_bgr) 
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

   //////////////////////////////////////////////////////////////////////////
   if (!g_sat_pos_is_calculated)
   {
      DisplatText("Place satellite and Earth marker");
      return; // to init program we need to have satel and planet 0
   }

   if (!g_initialization_is_done)
      DisplatText("Press Spacebar when ready");


   glEnable(GL_DEPTH_TEST);
   glMatrixMode( GL_MODELVIEW );
   //////////////////////////////////////////////////////////////////////////
   // draw planets
   for (uint i = 0; i < planets.size(); ++i)
   {
      if (!planets[i].m_is_on_scene)
         continue;

      float resultTransposedMatrix[16];
      for (int x=0; x<4; ++x)
         for (int y=0; y<4; ++y)
            resultTransposedMatrix[x*4+y] = planets[i].m_resultMatrix[y*4+x];

      glLoadMatrixf( resultTransposedMatrix );
      DrawPlanet(i);
   }

   //////////////////////////////////////////////////////////////////////////
   // draw satellites
   for (uint i = 0; i < satellites.size(); ++i)
   {
      TVector pos_sat_in_its_coor_sys;
      if (!satellites[i].m_is_on_scene)
         break;

      float resultTransposedMatrix[16];
      for (int x = 0; x < 4; ++x)
         for (int y = 0; y < 4; ++y)
            resultTransposedMatrix[x*4+y] = satellites[i].m_resultMatrix[y*4+x];

      glLoadMatrixf( resultTransposedMatrix );

	  if (!g_is_pending_reset)
	  {
        glPushMatrix();

        glTranslatef(satellites[i].m_pos.X(), satellites[i].m_pos.Y(), satellites[i].m_pos.Z());
		  DrawSatellite(i);

		  // draw vec betw planet and sat
		  //glBegin(GL_LINES);
		  //glVertex3f(0, 0, 0);
		  //glVertex3f(distance_vec_draw.X(), distance_vec_draw.Y(), distance_vec_draw.Z());
		  //glEnd();

		  // Draw velo and acc vectors
		  const double scale_for_velo_draw = 2;
		  const TVector velo_arrow_vec = satellites[i].m_velocity * scale_for_velo_draw;
		  glColor3f(0, 0, 1);
		  DrawArrow(0, 0, 0, velo_arrow_vec.X(), velo_arrow_vec.Y(), velo_arrow_vec.Z(), 0.001);

		  const double scale_for_acc_draw = 4;
		  const TVector acc_arrow_vec = satellites[i].m_acceleration_vec * scale_for_acc_draw;
		  glColor3f(1, 0, 0);
		  DrawArrow(0, 0, 0, acc_arrow_vec.X(), acc_arrow_vec.Y(), acc_arrow_vec.Z(), 0.001);
        glPopMatrix();

        DrawSatelliteTail(i);
	  }

	  //////////////////////////////////////////////////////////////////////////
	  // draw particles
     glTranslatef(satellites[i].m_pos.X(), satellites[i].m_pos.Y(), satellites[i].m_pos.Z());
	  DrawParticles();

   }
}

//////////////////////////////////////////////////////////////////////////
int main(int argc, char* argv[]) 
{
   GLFWwindow* window;

   if (!glfwInit())
      return -1;

   // initialize the window system
   /* Create a windowed mode window and its OpenGL context */
   window = glfwCreateWindow(g_initial_window_width, g_initial_window_height, "Gravity Explorer", NULL, NULL);
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
   InitTextures();
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
      UpdateState(markers);
      Display(window,           img_bgr);

      glfwSwapBuffers(window);
      glfwPollEvents();
   }

   gluDeleteQuadric(gp_quadratic);
   glfwTerminate();
   return 0;
}