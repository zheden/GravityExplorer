#pragma  once;

class StaticPlanet;
class Satellite;

cv::VideoCapture cap;
//camera settings
const int g_camera_width  = 640;
const int g_camera_height = 480;
const int g_virtual_camera_angle = 60;
unsigned char bkgnd[g_camera_width*g_camera_height*3];

GLenum g_is_spin_mode = GL_TRUE;
double g_hour_of_day = 0.0;
double g_num_hours_in_day = 3400.0; // to make slower rotatoin of planet
double g_animate_increment = 16;  // Time step for animation (hours)
double g_scale_factor_for_draw = 0.0003;
double g_last_time = 0;

TVector distance_vec_draw; // temp for debug

const double G = 0.00000066742;

std::vector<Satellite> satellites;
std::vector<StaticPlanet> planets;

GLUquadricObj * gp_quadratic; // storage for quadric objects
