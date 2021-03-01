#include <iostream>
#include <GL/glut.h>
#include <FreeImage.h>
#include <random>
#include <cmath>
#include <tuple>
#include <vector>

#define PI 3.14159265
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 500

using namespace std;

double dr = 0.0001;

//for texture mapping
static GLuint textureID;
GLubyte* textureData;
int imageWidth, imageHeight;

FIBITMAP* createBitMap(char const* filename) {
	FREE_IMAGE_FORMAT format = FreeImage_GetFileType(filename, 0);

	if (format == -1) {
		std::cout << "Could not find image: " << filename << " - Aborting." << std::endl;
		exit(-1);
	}

	if (format == FIF_UNKNOWN) {
		std::cout << "Couldn't determine file format - attempting to get from file extension..." << std::endl;
		format = FreeImage_GetFIFFromFilename(filename);

		if (!FreeImage_FIFSupportsReading(format)) {
			std::cout << "Detected image format cannot be read!" << std::endl;
			exit(-1);
		}
	}

	FIBITMAP* bitmap = FreeImage_Load(format, filename);

	int bitsPerPixel = FreeImage_GetBPP(bitmap);

	FIBITMAP* bitmap32;
	if (bitsPerPixel == 32) {
		std::cout << "Source image has " << bitsPerPixel << " bits per pixel. Skipping conversion." << std::endl;
		bitmap32 = bitmap;
	}
	else {
		std::cout << "Source image has " << bitsPerPixel << " bits per pixel. Converting to 32-bit colour." << std::endl;
		bitmap32 = FreeImage_ConvertTo32Bits(bitmap);
	}

	return bitmap32;
}

void generateTexture() {
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight,
		0, GL_BGRA_EXT, GL_UNSIGNED_BYTE, textureData);
}

void initTexture() {
	FIBITMAP* bitmap32 = createBitMap("texture.png");
	imageWidth = FreeImage_GetWidth(bitmap32);
	imageHeight = FreeImage_GetHeight(bitmap32);
	textureData = FreeImage_GetBits(bitmap32);
	generateTexture();
}

// contains information about a color (r, g, b, a)
class Color {
	
public:
	vector<float> c;

	// construct by passing float
	Color(float r, float g, float b, float a) {
		c.push_back(r);
		c.push_back(g);
		c.push_back(b);
		c.push_back(a);
	}

	// construct by passing RGB hex code
	Color(const char* r, const char* g, const char* b, float a = 0.3) {
		c.push_back((float)strtol(r, NULL, 16)/256);
		c.push_back((float)strtol(g, NULL, 16)/256);
		c.push_back((float)strtol(b, NULL, 16)/256);
		c.push_back(a);
	}
	float& operator[](const int i) {
		return c[i];
	}
};

// color pallete consists of four colors
class ColorSet {
public:
	vector<Color> cs;
	void push_color(Color x) { cs.push_back(x); }
	Color& operator[](const int i) {
		return cs[i];
	}
};

vector< ColorSet > color_palette_list;	// contains all ColorSet


class Airplane {
public:
	bool dir;            // 0:left, 1:right
	float v_x, v_y;      // speed
	float x, y;          // current position
	float angle;         // the angle between horizontal line and current velocity
	float size;			 // size of plane; half of one side

	Airplane(float xi, float yi, float vx, float vy) {
		v_x = vx; v_y = vy; x = xi; y = yi;
		angle = atan2(double(v_y) , double(v_x));
		size = 0.05;
	}

	void setpos(float x_, float y_) {
		x = x_;
		y = y_;
		angle = atan2(double(v_y) , double(v_x));
		
	}

	void setvel(float v_x_, float v_y_) {
		v_x = v_x_;
		v_y = v_y_;
	}

	float getvel() {
		return sqrt(v_x * v_x + v_y * v_y);
	}

	void draw() {
		glPushMatrix();
		
		glTranslatef(x + 0.08*cos(angle), y + 0.08*sin(angle), 0.0);
		glRotatef(float(angle * (360 / (2 * PI)) -45 ), 0.0f, 0.0f, 1.0f);
		glColor3f(0.0, 0.0, 0.0);
		glEnable(GL_TEXTURE_2D);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		glBindTexture(GL_TEXTURE_2D, textureID);
		glBegin(GL_QUADS);
		glTexCoord2f(1, 1); glVertex2f(size, size);
		glTexCoord2f(1, 0); glVertex2f(size, -size);
		glTexCoord2f(0, 0); glVertex2f(-size, -size);
		glTexCoord2f(0, 1); glVertex2f(-size, size);
		glEnd();
		glDisable(GL_TEXTURE_2D);
		glPopMatrix();
	}
};

class Cloud {
public:
	bool dir;				// 0: left, 1: right
	float v;				// speed
	double r, rmax;			// current radius, max radius
	float x, y;				// current position
	int cp;					// choose ColorSet at color_palette_list
	int cn;					// choose color at ColorSet
	bool increasing;		// becomes 0 if r exceeds rmax

	static bool choose_dir;

	Cloud(float xi, float yi, int color_set) {
		increasing = 1;

		// choose direction
		choose_dir = !choose_dir;
		dir = choose_dir;

		random_device rd;
		mt19937 gen(rd());
		
		// choose radius
		uniform_int_distribution<int> rad(3, 15);
		int temp = rad(gen);
		r = ((double)temp) * 0.005;
		uniform_int_distribution<int> radmax(temp+1, min(17, temp+10));
		rmax = ((double)radmax(gen)) * 0.005;

		// choose v
		uniform_int_distribution<int> speed(5, 20);
		v = (float)speed(gen)*0.5;

		// initialize x, y
		x = xi;
		y = yi;

		// choose color
		cp = color_set;
		uniform_int_distribution<int> c(0, 3);
		cn = c(gen);
	}

	// get the plane's velocity and update class members
	void update(float vx_p, float vy_p) {	
		// radius
		if (increasing) {
			if (r < rmax) r += dr;
			else {
				increasing = 0;
				r -= dr;
			}
		}
		else r -= dr;

		// x, y
		if (dir == 0) {			// left
			float dx = -vy_p * v * 0.01;
			float dy = vx_p * v * 0.01;
			x += dx;
			y += dy;
		}
		else {					// right
			float dx = vy_p * v * 0.01;
			float dy = -vx_p * v * 0.01;
			x += dx;
			y += dy;
		}
	}

	// draw Cloud object at the screen
	void draw() {
		glPushMatrix();
		glTranslatef(x, y, 0.0);
		auto col = color_palette_list[cp][cn];
		glColor4f(col[0], col[1], col[2], col[3]);
		glutSolidSphere(r, 15, 15);
		glPopMatrix();
	}
};

bool Cloud::choose_dir = 0;

float x_p = -0.8;		// initial position of the plane
float y_p = -1.0;
float vx_p = 0.0007;	// initial velocity of the plane
float vy_p = 0.0007;

float last_x_p = x_p;	// used to make new cloud
float last_y_p = y_p;

float x_m;				// position of mouse click
float y_m;
bool clicked = 0;		// control after mouse click
bool go_straight = 0;
bool make_circle = 0;
bool circle_dir;		// 0: CCW, 1: CW
float cr;				// circle radius
float d_angle;			// angle change during make_circle
float d_size;			// plane size change during make_circle

int ccs = 0;			// first ColorSet index

Airplane thatplane(x_p, y_p, vx_p, vy_p);
vector<Cloud> clouds;

// calculate distance between (x1,y1) and (x2,y2)
float calc_dist(float x1, float y1, float x2, float y2) {
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

// calculate distance between ax+by+c=0 and (x,y)
double calc_line_dist(double a, double b, double c, double x, double y){
	double numer = abs(a * x + b * y + c);
	double denom = sqrt(a * a + b * b);
	return numer / denom;
}

// calculate angle[rad] between two vectors
float calc_angle(float x1, float y1, float x2, float y2) {
	float dot = x1 * x2 + y1 * y2;
	float mag1 = sqrt(x1 * x1 + y1 * y1);
	float mag2 = sqrt(x2 * x2 + y2 * y2);
	float c = dot / mag1 / mag2;
	return acos(c);
}

void renderScene() {
	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1.f, 1.f, -1.f, 1.f, -10.0, 10.0);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	for (int j = 0; j < clouds.size(); j++) {
		clouds[j].draw();
	}
	
	thatplane.draw();
	

	glutSwapBuffers();
}

void processNormalKeys(unsigned char key, int x, int y) {
	if (key == 27 || key == 32) exit(0);
}

// check whether the plane is out of screen
bool out_of_screen() {
	if (x_p > 1.0 || y_p > 1.0 || x_p < -1.0 || y_p < -1.0) return true;
	return false;
}

void idle(void){
	// add one more cloud if the plane moved far enough from the position where previous cloud was made
	if (!out_of_screen() && calc_dist(last_x_p, last_y_p, x_p, y_p) > 0.06) {
		last_x_p = x_p;
		last_y_p = y_p;

		clouds.push_back(Cloud(x_p, y_p, ccs));

	}

	// update every cloud and erase it if its radius is less than 0
	for (int j = clouds.size()-1; j >= 0; j--) {
		clouds[j].update(vx_p, vy_p);
		if (clouds[j].r <= 0) clouds.erase(clouds.begin() + j);
	}

	// update the plane's position
	if (clicked) {
		// pos vector points from (x_p, y_p) to (x_m, y_m)
		float pos_x = x_m - x_p;
		float pos_y = y_m - y_p;
		float v = thatplane.getvel();

		if (!go_straight && !make_circle) {
			// dist btw extension of plane velocity and clicked point
			double d = calc_line_dist(-vy_p, vx_p, vy_p * x_p - vx_p * y_p, x_m, y_m);	
			if (d < 0.1) go_straight = 1;
			else {
				// turn dphi each time
				float new_angle = thatplane.angle;
				float dphi = v * 8;

				if (vx_p * pos_y - vy_p * pos_x > 0)
					new_angle += dphi;
				else
					new_angle -= dphi;

				double new_vx = v * cos((double)new_angle);
				double new_vy = v * sin((double)new_angle);
				vx_p = new_vx;
				vy_p = new_vy;
				thatplane.setvel(vx_p, vy_p);

				x_p += vx_p;
				y_p += vy_p;
				thatplane.setpos(x_p, y_p);
			}
		}

		if (go_straight) {
			// angle btw pos vector and plane velocity
			float a = calc_angle(pos_x, pos_y, vx_p, vy_p);
			if (PI / 2 - 0.1 < a && PI / 2 + 0.1 > a) {
				go_straight = 0;
				make_circle = 1;

				if (vx_p * pos_y - vy_p * pos_x > 0) circle_dir = 0;	// left
				else circle_dir = 1;									// right
				cr = calc_dist(x_p, y_p, x_m, y_m);

				d_angle = v / cr;
				d_size = 0.05 / (2 * PI / d_angle);
			}
			// go straight
			else {
				x_p += vx_p;
				y_p += vy_p;
				thatplane.setpos(x_p, y_p);
			}
		}

		// make circle
		if (make_circle) {
			// radius vector points from (x_m, y_m) to (x_p, y_p)
			float rad_x = -pos_x;
			float rad_y = -pos_y;
			float cur_angle = atan2(rad_y, rad_x);

			if (circle_dir == 0) cur_angle += d_angle;
			else cur_angle -= d_angle;
			rad_x = cr * cos(cur_angle);
			rad_y = cr * sin(cur_angle);

			vx_p = rad_x + x_m - x_p;
			vy_p = rad_y + y_m - y_p;
			thatplane.setvel(vx_p, vy_p);

			x_p = rad_x + x_m;
			y_p = rad_y + y_m;
			thatplane.setpos(x_p, y_p);

			thatplane.size -= d_size;
			if (thatplane.size < 0) {
				// move it out of screen
				x_p = 2.0;
				y_p = 2.0;
				clicked = 0;
				make_circle = 0;
			}
			
		}
	}
	else {
		if (!out_of_screen()) {
			x_p += vx_p;
			y_p += vy_p;
			thatplane.setpos(x_p, y_p);
		}
		else {
			if (clouds.size() < 3) {
				random_device rd;
				mt19937 gen(rd());

				uniform_int_distribution<int> side(0, 3);	// choose which side the plane will appear
				uniform_int_distribution<int> pos(-8, 8);

				uniform_int_distribution<int> speed(8, 15);
				float s = (float)speed(gen) / 10000;
				uniform_int_distribution<int> angle(30, 150);
				float a = (float)angle(gen);
				float a1 = a * PI / 180;
				float a2 = (a - 90) * PI / 180;

				switch (side(gen)) {
				case 0:	// left side
					x_p = -1.0;
					y_p = (float)pos(gen) / 10;
					vx_p = s * cos(a2);
					vy_p = s * sin(a2);
					break;
				case 1:	// upper side
					x_p = (float)pos(gen) / 10;
					y_p = 1.0;
					vx_p = s * cos(a1);
					vy_p = -s * sin(a1);
					break;
				case 2:	// right side
					x_p = 1.0;
					y_p = (float)pos(gen) / 10;
					vx_p = -s * cos(a2);
					vy_p = s * sin(a2);
					break;
				case 3:	// lower side
					x_p = (float)pos(gen) / 10;
					y_p = -1.0;
					vx_p = s * cos(a1);
					vy_p = s * sin(a1);
					break;
				}
				last_x_p = x_p;
				last_y_p = y_p;
				thatplane.setpos(x_p, y_p);
				thatplane.setvel(vx_p, vy_p);
				thatplane.size = 0.05;

				uniform_int_distribution<int> choose_ColorSet(0, color_palette_list.size() - 1);
				int temp = ccs;
				// to prevent same color set
				while (temp == ccs) {
					temp = choose_ColorSet(gen);
				}
				ccs = temp;

			}
			else {
				x_p += vx_p;
				y_p += vy_p;
				thatplane.setpos(x_p, y_p);
			}
		}
	}
	
	glutPostRedisplay();
}

void processMouse(int button, int state, int x, int y) {
	if (button == GLUT_LEFT_BUTTON && state == GLUT_UP && !clicked && !out_of_screen()) {
		x_m = (float)x/(SCREEN_WIDTH/2)-1;
		y_m = -((float)y/(SCREEN_HEIGHT/2)-1);
		clicked = 1;
	}
}

// save ColorSet
void set_color_palette() {
	// 2 color sets below are from https://digitalsynopsis.com/design/flat-color-palettes/
	ColorSet rednpink;
	rednpink.push_color(Color("EE", "45", "40"));
	rednpink.push_color(Color("C7", "27", "41"));
	rednpink.push_color(Color("80", "13", "36"));
	rednpink.push_color(Color("51", "0A", "32"));
	color_palette_list.push_back(rednpink);

	ColorSet greennpink;
	greennpink.push_color(Color("8F", "B9", "A8"));
	greennpink.push_color(Color("FE", "FA", "D4", 0.9));
	greennpink.push_color(Color("FC", "D0", "BA"));
	greennpink.push_color(Color("F1", "82", "8D"));	
	color_palette_list.push_back(greennpink);
	
	// from https://avemateiu.com/color-palettes/color-palette-208/
	ColorSet pastelpink;
	pastelpink.push_color(Color("FB", "D1", "D3"));
	pastelpink.push_color(Color("F1", "98", "AF"));
	pastelpink.push_color(Color("EB", "B2", "D6"));
	pastelpink.push_color(Color("9F", "81", "CD"));
	color_palette_list.push_back(pastelpink);

	// from http://girlybusinesscards.blogspot.com/2014/09/hex-code-pantone-color-palette-for.html
	ColorSet blue;
	blue.push_color(Color("9D", "C6", "D8"));
	blue.push_color(Color("00", "B3", "CA"));
	blue.push_color(Color("7D", "D0", "B6"));
	blue.push_color(Color("1D", "4E", "89"));
	color_palette_list.push_back(blue);

	// from https://www.schemecolor.com/retro.php
	ColorSet olive;
	olive.push_color(Color("83", "B7", "99"));
	olive.push_color(Color("E2", "CD", "6D"));
	olive.push_color(Color("C2", "B2", "8F"));
	olive.push_color(Color("E4", "D8", "B4"));
	color_palette_list.push_back(olive);


}

int main(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	glutCreateWindow("clouds");
	initTexture();

	set_color_palette();

	glutDisplayFunc(renderScene);
	glutKeyboardFunc(processNormalKeys);
	glutIdleFunc(idle);
	glutMouseFunc(processMouse);
	glutMainLoop();

	return EXIT_SUCCESS;
}
