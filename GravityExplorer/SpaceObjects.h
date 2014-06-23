#pragma once;

//////////////////////////////////////////////////////////////////////////
class SpaceObject
{
public:
   SpaceObject(const TVector& i_pos, double i_mass, GLuint i_texID):
      m_pos(i_pos), m_mass(i_mass), m_texID(i_texID) {};

   TVector m_pos;
   double m_mass;
   GLuint m_texID;
};

//////////////////////////////////////////////////////////////////////////
class StaticPlanet : public SpaceObject
{
public:
   StaticPlanet(const TVector& i_pos, double i_mass, GLuint i_texID, int i_code) :
      SpaceObject(i_pos, i_mass, i_texID), m_radius_scale(1), m_code(i_code), m_is_on_scene(false) {};

   double m_radius_scale;
   bool m_is_on_scene;
   int m_code; // marker code
   float m_resultMatrix[16];
};

//////////////////////////////////////////////////////////////////////////
class Satellite: public SpaceObject
{
public:
   Satellite(const TVector& i_velocity, const TVector& i_pos, double i_mass /*Lib3dsFile* ip_file*/) :
      SpaceObject(i_pos, i_mass, NULL), m_velocity(i_velocity), acceleration_vec(0, 0, 0) {};

   TVector m_velocity;
   TVector acceleration_vec;
};




//////////////////////////////////////////////////////////////////////////
void rotateAroundXaxis(double i_angle)
{
   if (satellites.empty())
      return;

   const double ynew = satellites[0].m_velocity.Y() * cos(i_angle) - satellites[0].m_velocity.Z() * sin(i_angle);
   const double znew = satellites[0].m_velocity.Y() * sin(i_angle) + satellites[0].m_velocity.Z() * cos(i_angle);
   satellites[0].m_velocity.Y() = ynew;
   satellites[0].m_velocity.Z() = znew;
}

//////////////////////////////////////////////////////////////////////////
void rotateAroundYaxis(double i_angle)
{
   if (satellites.empty())
      return;

   const double xnew = satellites[0].m_velocity.X() * cos(i_angle) + satellites[0].m_velocity.Z() * sin(i_angle);
   const double znew = -satellites[0].m_velocity.X() * sin(i_angle) + satellites[0].m_velocity.Z() * cos(i_angle);
   satellites[0].m_velocity.X() = xnew;
   satellites[0].m_velocity.Z() = znew;
}

//////////////////////////////////////////////////////////////////////////
void rotateAroundZaxis(double i_angle)
{
   if (satellites.empty())
      return;

   const double xnew = satellites[0].m_velocity.X() * cos(i_angle) - satellites[0].m_velocity.Y() * sin(i_angle);
   const double ynew = satellites[0].m_velocity.X() * sin(i_angle) + satellites[0].m_velocity.Y() * cos(i_angle);
   satellites[0].m_velocity.X() = xnew;
   satellites[0].m_velocity.Y() = ynew;
}