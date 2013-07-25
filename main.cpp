/*
 * main.cpp
 *
 *  Created on: Jun 11, 2013
 *      Author: Reza
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#include <GL/glu.h>
#include <GL/gl.h>
#include "imageloader.h"
#include "vec3f.h"
#endif

static GLfloat spin = 0.0;
float angle = 0;
using namespace std;

float lastx, lasty;
GLint stencilBits;
static int camx = 100;
static int camy = 50;
static int camz = 50;

float rot = 0;


class Rumput {
private:
	int w; //lebar
	int l; //panjang
	float** t; //tinggi
	Vec3f** acx;
	bool komposisi;
public:
	Rumput(int w2, int l2) {
		w = w2;
		l = l2;
		t = new float*[l];
		for (int i = 0; i < l; i++) {
			t[i] = new float[w];}
		acx = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			acx[i] = new Vec3f[w];}
		komposisi = false;}
	~Rumput() {
		for (int i = 0; i < l; i++) {
			delete[] t[i];}
		delete[] t;
		for (int i = 0; i < l; i++) {
			delete[] acx[i];}
		delete[] acx;}
	int width() {
		return w;}
	int length() {
		return l;}

	//Mengatur tinggi (x, z) terhadap y
	void setHeight(int x, int z, float y) {
		t[z][x] = y;
		komposisi = false;}

	//Mengembalikan nilai tinggi (x, z)
	float getHeight(int x, int z) {
		return t[z][x];}

	//Menghitung var acx, jika belum dihitung
	void computeNormals() {
		if (komposisi) {
			return;}
		//Menghitung var acx
		Vec3f** acx2 = new Vec3f*[l];
		for (int i = 0; i < l; i++) {
			acx2[i] = new Vec3f[w];}

		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum(0.0f, 0.0f, 0.0f);
				Vec3f out;
				if (z > 0) {
					out = Vec3f(0.0f, t[z - 1][x] - t[z][x], -1.0f);}
				Vec3f in;
				if (z < l - 1) {
					in = Vec3f(0.0f, t[z + 1][x] - t[z][x], 1.0f);}
				Vec3f left;
				if (x > 0) {
					left = Vec3f(-1.0f, t[z][x - 1] - t[z][x], 0.0f);}
				Vec3f right;
				if (x < w - 1) {
					right = Vec3f(1.0f, t[z][x + 1] - t[z][x], 0.0f);}
				if (x > 0 && z > 0) {
					sum += out.cross(left).normalize();}
				if (x > 0 && z < l - 1) {
					sum += left.cross(in).normalize();}
				if (x < w - 1 && z < l - 1) {
					sum += in.cross(right).normalize();}
				if (x < w - 1 && z > 0) {
					sum += right.cross(out).normalize();}
				acx2[z][x] = sum;}}

		// memperhalus var acx
		const float FALLOUT_RATIO = 0.5f;
		for (int z = 0; z < l; z++) {
			for (int x = 0; x < w; x++) {
				Vec3f sum = acx2[z][x];
				if (x > 0) {
					sum += acx2[z][x - 1] * FALLOUT_RATIO;}
				if (x < w - 1) {
					sum += acx2[z][x + 1] * FALLOUT_RATIO;}
				if (z > 0) {
					sum += acx2[z - 1][x] * FALLOUT_RATIO;}
				if (z < l - 1) {
					sum += acx2[z + 1][x] * FALLOUT_RATIO;}
				if (sum.magnitude() == 0) {
					sum = Vec3f(0.0f, 1.0f, 0.0f);}
				acx[z][x] = sum;}}
		for (int i = 0; i < l; i++) {
			delete[] acx2[i];
		}
		delete[] acx2;
		komposisi = true;
	}

	//Mengembalikan nilai (x, z)
	Vec3f getNormal(int x, int z) {
		if (!komposisi) {
			computeNormals();
		}
		return acx[z][x];
	}
};
//Tutup Class

void initRendering() { // Inisialisasi
	//glenable : Fungsi mengaktifkan atau menonaktifkan kemampuan OpenGL.
	glEnable(GL_DEPTH_TEST);//Melakukan perbandingan kedalaman dan memperbarui kedalaman
	glEnable(GL_COLOR_MATERIAL);//Fungsi menyebabkan warna bahan untuk melacak warna saat ini
	glEnable(GL_LIGHTING);//
	glEnable(GL_LIGHT0);//Fungsi mengembalikan sumber cahaya nilai parameter.
	glEnable(GL_NORMALIZE);//
	glShadeModel(GL_SMOOTH);//Fungsi memilih shading datar atau halus.
}

// Loads Rumput berdasarkan tinggi. Tinggi rumput dihitung dari jarak
// -tinggi / 2 kepada tinggi / 2.
// Loads terain di procedure inisialisasi
Rumput* loadRumput(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Rumput* t = new Rumput(image->width, image->height);
	for (int y = 0; y < image->height; y++) {
		for (int x = 0; x < image->width; x++) {
			unsigned char color = (unsigned char) image->pixels[3
					* (y * image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);}}
	delete image;
	t->computeNormals();
	return t;}

float _angle = 60.0f;

// Buat tipe data terain
Rumput* _Rumput;
Rumput* _RumputTanah;
Rumput* _RumputAir;

const GLfloat light_ambient[] = { 0.3f, 0.3f, 0.3f, 1.0f }; //mengatur
const GLfloat light_diffuse[] = { 0.7f, 0.7f, 0.7f, 1.0f };
const GLfloat light_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat light_position[] = { 1.0f, 1.0f, 1.0f, 1.0f };

const GLfloat light_ambient2[] = { 0.3f, 0.3f, 0.3f, 0.0f };
const GLfloat light_diffuse2[] = { 0.3f, 0.3f, 0.3f, 0.0f };

const GLfloat mat_ambient[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
const GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
const GLfloat high_shininess[] = { 100.0f };

void load_BMP_texture(char *filename) {

    FILE *file;
    short int bpp;
    short int planes;
    long size;
    unsigned int texture;

    long imwidth;
    long imheight;
    char *imdata;

    file = fopen(filename, "rb");
    fseek(file, 18, SEEK_CUR);

    fread(&imwidth, 4, 1, file);
    fread(&imheight, 4, 1, file);
    size = imwidth * imheight * 3;

    fread(&bpp, 2, 1, file);
    fread(&planes, 2, 1, file);

    fseek(file, 24, SEEK_CUR);
    imdata = (char *)malloc(size);

    fread(imdata, size, 1, file);

    char temp;
    for(long i = 0; i < size; i+=3){
        temp = imdata[i];
        imdata[i] = imdata[i+2];
        imdata[i+2] = temp;
    }

    fclose(file);

    glGenTextures(1, &texture); // then we need to tell OpenGL that we are generating a texture
    glBindTexture(GL_TEXTURE_2D, texture); // now we bind the texture that we are working with

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imwidth, imheight, 0, GL_RGB, GL_UNSIGNED_BYTE, imdata);

    free(imdata); // free the texture
}

void cleanup() {
	delete _Rumput;
	delete _RumputTanah;}

void displayRumput(Rumput *Rumput, GLfloat r, GLfloat g, GLfloat b) {
	float scale = 350.0f / max(Rumput->width() - 1, Rumput->length() - 1);
	glScalef(scale, scale, scale);
	glTranslatef(-(float) (Rumput->width() - 1) / 2, 0.0f,
			-(float) (Rumput->length() - 1) / 2);

	glColor3f(r, g, b);
	for (int z = 0; z < Rumput->length() - 1; z++) {
		//Makes OpenGL draw a triangle at every three consecutive vertices
		glBegin(GL_TRIANGLE_STRIP);//Menggambar segitiga
		for (int x = 0; x < Rumput->width(); x++) {
			Vec3f normal = Rumput->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, Rumput->getHeight(x, z), z);
			normal = Rumput->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, Rumput->getHeight(x, z + 1), z + 1);
		}
		glEnd();//fungsi membatasi simpul dari primitif atau kelompok seperti primitif
	}

}

void tempatTidur(){
     glPushMatrix();//Membuat baris kode diantaranya menjadi tidak berlaku untuk bagian luar.
       glTranslatef(-15.0,-8.5,-12.0);//digunakan untuk merubah titik tengah sumbu koordinat
       glScalef(6.0,1.0,9.0);//Skalasi merupakan bentuk transformasi yang dapat mengubah ukuran (besar-kecil) suatu objek.
       glColor3f(0.0980, 0.0608, 0.0077);//Mengatur warna berdasarkan warna desimal
       glutSolidCube(1.5);
    glPopMatrix();//Membuat baris kode diantaranya menjadi tidak berlaku untuk bagian luar.
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glPushMatrix();
       glTranslatef(-15.0,-8.5,-12.0);
       glColor3ub (255, 250, 250);
       glScalef(6.0,2.0,10.0);
       glutSolidCube(1.3);
    glPopMatrix();
    // bantal kiri
    glPushMatrix();
    	glRotated(-90, 0.0, 1.0, 0.0);
    	glScaled(0.4, 0.1, 0.6);
    	glTranslatef(-43, -70, 28);
    	glColor3ub (255, 250, 250);
    	glutSolidCube(5.0);
    glPopMatrix();
    // bantal kanan
    glPushMatrix();
    	glRotated(-90, 0.0, 1.0, 0.0);//Rotasi merupakan bentuk transformasi yang digunakan untuk memutar posisisuatu benda
    	glScaled(0.4, 0.1, 0.6);//fungsi kalikan matriks saat ini dengan matriks skala umum
    	glTranslatef(-43, -70, 22);
    	glColor3ub (255, 250, 250);
    	glutSolidCube(5.0);
    glPopMatrix();//
}

void sofaPanjang(){
	// bawah sofa
	glRotated(-90, 0.0, 1.0, 0.0);
	glPushMatrix();
		glScaled(1.5, 0.5, 5);
		glTranslatef(1, -17, 0);
		glColor3f(0.0980, 0.0608, 0.0077);
	    glutSolidCube(5.0);
    glPopMatrix();
    // senderan sofa
	glRotated(-90, 0.0, 1.0, 0.0);
	glPushMatrix();
		glScaled(5, 1.5, 0.5);
		glTranslatef(0, -4, -11);
	    glColor3ub(255, 250, 250);
	    glutSolidCube(5.0);
    glPopMatrix();
    // sisi kanan sofa
	glRotated(-90, 0.0, 1.0, 0.0);
	glPushMatrix();
		glScaled(2, 1, 0.5);
		glTranslatef(-0.8, -7, 25);
		glColor3f(0.0980, 0.0608, 0.0077);
	    glutSolidCube(5.0);
    glPopMatrix();
    //sisi kiri sofa
    glPushMatrix();
		glScaled(2, 1, 0.5);
		glTranslatef(-0.8, -7, -25);
		glColor3f(0.0980, 0.0608, 0.0077);
	    glutSolidCube(5.0);
    glPopMatrix();
    // bantal sofa
	glPushMatrix();
		glScaled(1, 0.1, 4);
		glTranslatef(-1, -70, 0);
	    glColor3ub(255, 250, 250);
	    glutSolidCube(5.0);
    glPopMatrix();
}

void pintuUtama(){
	glPushMatrix();
		glScaled(3, 5, 0.8);
		glTranslatef(3, 2.5, 76);
		glColor3ub (255, 250, 250);
		glutSolidCube(5.0);
		glPushMatrix();
			glTranslatef(-5.055, 0, 0);
			glColor3ub (255, 250, 250);
			glutSolidCube(5.0);
		glPopMatrix();
	glPopMatrix();

	glPushMatrix();
		glScaled(0.2, 0.7, 1);
		glTranslatef(0, 20, 60.88);
		glColor3ub(000, 000, 000);
		glutSolidCube(5.0);
	glPopMatrix();

	glPushMatrix();
		glScaled(0.2, 0.7, 1);
		glTranslatef(15, 20, 60.88);
		glColor3ub(000, 000, 000);
		glutSolidCube(5.0);
	glPopMatrix();
}

void tangga(){
	glRotated(-90, 0.0, 1.0, 0.0);
	glPushMatrix();
		glScaled(0.7, 0.5, 2.5);
		glTranslatef(-30, 5, 12);
		glColor3ub(138, 138, 138);
	    glutSolidCube(5.0);
    glPopMatrix();
	glPushMatrix();
	glScaled(0.7, 1.0, 2.5);
		glTranslatef(-35, 3.8, 12);
	    glutSolidCube(5.0);
    glPopMatrix();
    glPushMatrix();
    glScaled(0.7, 1.5, 2.5);
    	glTranslatef(-40, 3.5, 12);
	    glutSolidCube(5.0);
    glPopMatrix();
    glPushMatrix();
    glScaled(0.7, 2, 2.5);
    	glTranslatef(-45, 3.2, 12);
	    glutSolidCube(5.0);
    glPopMatrix();
    glPushMatrix();
    glScaled(0.7, 2.5, 2.5);
    	glTranslatef(-50, 3, 12);
	    glutSolidCube(5.0);
    glPopMatrix();
    glPushMatrix();
    glScaled(0.7, 3, 2.5);
	    glTranslatef(-55, 2.8, 12);
	    glutSolidCube(5.0);
    glPopMatrix();
    glPushMatrix();
    glScaled(2, 3.5, 2.5);
	    glTranslatef(-22.5, 2.7, 12);
	    glutSolidCube(5.0);
    glPopMatrix();
    glPushMatrix();
    glRotated(-90, 0.0, 1.0, 0.0);
    glScaled(0.7, 4, 2.5);
	    glTranslatef(31, 2.6, 19.5);
	    glutSolidCube(5.0);
    glPopMatrix();
    glPushMatrix();
    glRotated(-90, 0.0, 1.0, 0.0);
    glScaled(0.7, 4.5, 2.5);
	    glTranslatef(27, 2.4, 19.5);
	    glutSolidCube(5.0);
    glPopMatrix();
    glPushMatrix();
    glRotated(-90, 0.0, 1.0, 0.0);
    glScaled(0.7, 5, 2.5);
	    glTranslatef(23, 2.4, 19.5);
	    glutSolidCube(5.0);
    glPopMatrix();
    glPushMatrix();
    glRotated(-90, 0.0, 1.0, 0.0);
    glScaled(0.7, 5.5, 2.5);
	    glTranslatef(19, 2.4, 19.5);
	    glutSolidCube(5.0);
    glPopMatrix();
    glPushMatrix();
    glRotated(-90, 0.0, 1.0, 0.0);
    glScaled(0.7, 6, 2.5);
	    glTranslatef(15, 2.4, 19.5);
	    glutSolidCube(5.0);
    glPopMatrix();

    glPushMatrix();
    glRotated(-90, 0.0, 1.0, 0.0);
    glScaled(0.7, 6.5, 2.5);
	    glTranslatef(11, 2.4, 19.5);
	    glutSolidCube(5.0);
    glPopMatrix();

    glPushMatrix();
    glRotated(-90, 0.0, 1.0, 0.0);
    glScaled(0.7, 7, 2.5);
	    glTranslatef(7, 2.4, 19.5);
	    glutSolidCube(5.0);
    glPopMatrix();

    glPushMatrix();
    glRotated(-90, 0.0, 1.0, 0.0);
    glScaled(0.7, 7.5, 2.5);
	    glTranslatef(3, 2.4, 19.5);
	    glutSolidCube(5.0);
    glPopMatrix();
}

void lampuMeja() {
	glPushMatrix();
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3d(0.0, 1.0, 0.0);
		glRotated(-90.0, 1.0, 0.0, 0.0);
		glColor3ub(255, 255, 000);
		glutSolidCone(15.0, 20.0, 15, -10);
	glPopMatrix();

    glPushMatrix();
		glRotatef(90, 1, 0, 0);
		glTranslatef(0.0, 0, -2);
		glScaled(1.4, 2, 30);
		glColor3ub(255, 250, 250);
		glutSolidTorus(0.4, 0.4, 60, 80);
    glPopMatrix();

	 glPushMatrix();
		glColor3ub(46, 79, 219);
		glTranslatef(0, -9.1, -0.05);
		glScaled(2, 0.1, 2);
		glColor3ub(255, 250, 250);
		glutSolidSphere(6, 25, 25);
	 glPopMatrix();

}

void meja(void){
    glPushMatrix();
       glTranslatef(0.0,-5.0,0.0);
       glColor3f(0.0980, 0.0608, 0.0077);
       glScalef(6.0,0.1,8.0);
       glutSolidCube(1.0);
    glPopMatrix();

    glPushMatrix();
       glTranslatef(-2.5,-7.0,3.5);
       glScalef(1.0,4.0,1.0);
       glutSolidCube(1.0);
    glPopMatrix();

    glPushMatrix();
       glTranslatef(2.5,-7.0,3.5);
       glScalef(1.0,4.0,1.0);
       glutSolidCube(1.0);
    glPopMatrix();

    glPushMatrix();
       glTranslatef(-2.5,-7.0,-3.5);
       glScalef(1.0,4.0,1.0);
       glutSolidCube(1.0);
    glPopMatrix();

    glPushMatrix();
       glTranslatef(2.5,-7.0,-3.5);
       glScalef(1.0,4.0,1.0);
       glutSolidCube(1.0);
    glPopMatrix();
}

void kursi(void) {

    // Batang Tiang Kanan
    glPushMatrix();
    glScaled(0.06, 0.2, 0.06);
    glTranslatef(43, 0, 380.5);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();
    // Batang Tiang Kiri
    glPushMatrix();
    glScaled(0.06, 0.2, 0.06);
    glTranslatef(3, 0, 380.5);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();
    // Batang depan knan
    glPushMatrix();
    glScaled(0.06, 0.2, 0.06);
    glTranslatef(43, 0, 390.5);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();
    // Batang Depan Kiri
    glPushMatrix();
    glScaled(0.06, 0.2, 0.06);
    glTranslatef(3, 0, 390.5);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1, 1, 1);
    glutSolidCube(5.0);
    glPopMatrix();
    // atas kursi
    glPushMatrix();
    glScaled(0.6, 0.05, 0.3);
    glTranslatef(2.4, 8, 77);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();
}

void bangunan(void) {

//lantai 1
	glPushMatrix();
	glScaled(1.115, 0.03, 2);
	glTranslatef(0.0, 0, 0.38);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(255, 255, 255);
	glutSolidCube(5.0);
	glPopMatrix();

// lantai 2
	glPushMatrix();
	glScaled(1.015, 0.03, 1.64);
	glTranslatef(0.0, 80, 0.57);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(255, 255, 255);
	glutSolidCube(5.0);
	glPopMatrix();
	glPushMatrix();
	glScaled(0.5, 0.03, 0.15);
	glTranslatef(2.56, 80, -23.6);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(255, 255, 255);
	glutSolidCube(5.0);
	glPopMatrix();

//lantai 3
	glPushMatrix();
	glScaled(0.95, 0.03, 1.8);
	glTranslatef(0.0,160, 0.3);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(138, 138, 138);
	glutSolidCube(5.0);
	glPopMatrix();

//lapisan lantai 3
	glPushMatrix();
	glScaled(0.95, 0.02, 1.8);
	glTranslatef(0.0,246, 0.3);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(000, 191, 255);
	glutSolidCube(5.0);
	glPopMatrix();

//Dinding Kiri Bawah
	glPushMatrix();
	glScaled(0.035, 0.5, 1.6);
	glTranslatef(-70.0, 2.45, 0.0);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(000, 191, 255);
	glutSolidCube(5.0);
	glPopMatrix();

//	//Dinding Kanan Bawah
//	glPushMatrix();
//	glScaled(0.035, 0.5, 1.6);
//	glTranslatef(70.0, 2.45, 0.0);
//	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
//	glColor3f(0.4613, 0.4627, 0.4174);
//	glutSolidCube(5.0);
//	glPopMatrix();
//
//	//Dinding Kanan Atas
//	glPushMatrix();
//	glScaled(0.035, 0.5, 1.8);
//	glTranslatef(70.0, 7.45, 0.3);
//	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
//	glColor3ub(000, 191, 255);
//	glutSolidCube(5.0);
//	glPopMatrix();

//Dinding Kiri Atas
	glPushMatrix();
	glScaled(0.035, 0.5, 1.8);
	glTranslatef(-70.0, 7.45, 0.3);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(000, 191, 255);
	glutSolidCube(5.0);
	glPopMatrix();

//Dinding Belakang bawah
	glPushMatrix();
	glScaled(1.015, 0.5, 0.07);
	glTranslatef(0, 2.45,-58);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(000, 191, 255);
	glutSolidCube(5.0);
	glPopMatrix();

//Dinding Belakang atas
	glPushMatrix();
	glScaled(1.015, 0.5, 0.07);
	glTranslatef(0, 7.45,-58);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(000, 191, 255);
	glutSolidCube(5.0);
	glPopMatrix();

//Dinding Depan bawah
	glPushMatrix();
	glScaled(1.015, 0.5, 0.035);
	glTranslatef(0, 2.25,116);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(000, 191, 255);
	glutSolidCube(5.0);
	glPopMatrix();

	//Dinding Depan atas
		glPushMatrix();
		glScaled(1.015, 0.5, 0.035);
		glTranslatef(0, 7.45, 142);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(000, 191, 255);
		glutSolidCube(5.0);
		glPopMatrix();

// HIASAN DINDING
	//background
	glPushMatrix();
	glScaled(0.35, 0.5, 0.035);
	glTranslatef(1, 7.2,-112);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(138, 138, 138);
	glutSolidCube(5.0);
	glPopMatrix();
	//strip 1
	glPushMatrix();
	glScaled(0.017,0.33, 0.035);
	glTranslatef(-16.6, 12,-110);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub (255, 255 ,255);
	glutSolidCube(5.0);
	glPopMatrix();
	//strip 2
	glPushMatrix();
	glScaled(0.017,0.33, 0.035);
	glTranslatef(-6.6, 12,-110);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub (255, 255 ,255);
	glutSolidCube(5.0);
	glPopMatrix();
	//strip 3
	glPushMatrix();
	glScaled(0.017,0.33, 0.035);
	glTranslatef(3.6, 12,-110);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub (255, 255 ,255);
	glutSolidCube(5.0);
	glPopMatrix();

// JENDELA ATAS KANAN
	// bingkai atas
	glPushMatrix();
	glScaled(0.08, 0.017, 0.035);
	glTranslatef(22.5, 265,-112);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(255, 255, 240);
	glutSolidCube(5.0);
	glPopMatrix();
	// bingkai bawah
	glPushMatrix();
	glScaled(0.08, 0.017, 0.035);
	glTranslatef(22.5, 183,-112);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(255, 255, 240);
	glutSolidCube(5.0);
	glPopMatrix();
	// bingkai kiri
	glPushMatrix();
	glScaled(0.017,0.28, 0.035);
	glTranslatef(96.6, 13.5,-112);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(255, 255, 240);
	glutSolidCube(5.0);
	glPopMatrix();
	// bingkai kanan
	glPushMatrix();
	glScaled(0.017,0.28, 0.035);
	glTranslatef(115.1, 13.5,-112);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(255, 255, 240);
	glutSolidCube(5.0);
	glPopMatrix();

	// MEJA KIRI
		// tatakan meja
		glPushMatrix();
		glRotated(90, 1.0, 0.0, 0.0);
		glScaled(0.98, 0.48, 0.08);
		glTranslatef(-1.5, 9.0, -36);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(255, 222, 173);
		glutSolidCube(1.0);
		glPopMatrix();
		// kaki meja kiri belakang
		glPushMatrix();
		glScaled(0.06, 0.5,0.06);
		glTranslatef(-30, 5.2, 75.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(255, 222, 173);
		glutSolidCube(1.0);
		glPopMatrix();
		// kaki meja kiri depan
		glPushMatrix();
		glScaled(0.06, 0.5,0.06);
		glTranslatef(-20, 5.2, 75.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(255, 222, 173);
		glutSolidCube(1.0);
		glPopMatrix();
		// kaki meja kanan depan
		glPushMatrix();
		glScaled(0.06, 0.5,0.06);
		glTranslatef(-20, 5.2, 68.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(255, 222, 173);
		glutSolidCube(1.0);
		glPopMatrix();
		// kaki meja kanan belakang
		glPushMatrix();
		glScaled(0.06, 0.5,0.06);
		glTranslatef(-30, 5.2, 68.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(255, 222, 173);
		glutSolidCube(1.0);
		glPopMatrix();

		// MEJA KIRI
			// tatakan meja
			glPushMatrix();
			glRotated(90, 1.0, 0.0, 0.0);
			glScaled(0.98, 0.48, 0.08);
			glTranslatef(-1.5, 9.0, -36);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glColor3f(0.0980, 0.0608, 0.0077);
			glutSolidCube(1.0);
			glPopMatrix();
			// kaki meja kiri belakang
			glPushMatrix();
			glScaled(0.06, 0.5,0.06);
			glTranslatef(-30, 5.2, 75.5);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glutSolidCube(1.0);
			glPopMatrix();
			// kaki meja kiri depan
			glPushMatrix();
			glScaled(0.06, 0.5,0.06);
			glTranslatef(-20, 5.2, 75.5);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glutSolidCube(1.0);
			glPopMatrix();
			// kaki meja kanan depan
			glPushMatrix();
			glScaled(0.06, 0.5,0.06);
			glTranslatef(-20, 5.2, 68.5);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glutSolidCube(1.0);
			glPopMatrix();
			// kaki meja kanan belakang
			glPushMatrix();
			glScaled(0.06, 0.5,0.06);
			glTranslatef(-30, 5.2, 68.5);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glutSolidCube(1.0);
			glPopMatrix();

		// MEJA KIRI
			// tatakan meja
			glPushMatrix();
			glRotated(90, 1.0, 0.0, 0.0);
			glScaled(0.98, 0.48, 0.08);
			glTranslatef(-1.50, 2.5, -36);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glColor3f(0.0980, 0.0608, 0.0077);
			glutSolidCube(1.0);
			glPopMatrix();
			// kaki meja kiri belakang
			glPushMatrix();
			glScaled(0.06, 0.5,0.06);
			glTranslatef(-30, 5.2, 23);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glutSolidCube(1.0);
			glPopMatrix();
			// kaki meja kiri depan
			glPushMatrix();
			glScaled(0.06, 0.5,0.06);
			glTranslatef(-20, 5.2, 23);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glutSolidCube(1.0);
			glPopMatrix();
			// kaki meja kanan depan
			glPushMatrix();
			glScaled(0.06, 0.5,0.06);
			glTranslatef(-20, 5.2, 16.8);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glutSolidCube(1.0);
			glPopMatrix();
			// kaki meja kanan belakang
			glPushMatrix();
			glScaled(0.06, 0.5,0.06);
			glTranslatef(-30, 5.2, 16.8);
			glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
			glutSolidCube(1.0);
			glPopMatrix();

			//	LEMARI
				glPushMatrix();
				glRotated(-90, 0.0, 1.0, 0.0);
				glScaled(0.18, 0.35, 0.16);
				glTranslatef(-12, 9.5, 13);
				glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
				glColor3f(0.0980, 0.0608, 0.0077);
				glutSolidCube(5.0);
				glPopMatrix();

//	LAMPU ATAS KANAN
	glPushMatrix();
	glScaled(0.05, 0.05, 0.05);
	glTranslatef(34.5, 95.4, 10);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); //untuk memunculkan warna
	glColor3ub(255, 255, 000);
	glutSolidSphere(2.0,20,50);
	glPopMatrix();

//  LAMPU ATAS KIRI
	glPushMatrix();
	glScaled(0.05, 0.05, 0.05);
	glTranslatef(-32.5, 95.4, 10);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(255, 255, 000);
	glutSolidSphere(2.0,20,50);
	glPopMatrix();

//	LAMPU BAWAH KANAN
	glPushMatrix();
	glScaled(0.05, 0.05, 0.05);
	glTranslatef(34.5, 47, 10);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE); //untuk memunculkan warna
	glColor3ub(255, 255, 000);
	glutSolidSphere(2.0,20,50);
	glPopMatrix();

//  LAMPU BAWAH KIRI
	glPushMatrix();
	glScaled(0.05, 0.05, 0.05);
	glTranslatef(-32.5, 47, 10);
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	glColor3ub(255, 255, 000);
	glutSolidSphere(2.0,20,50);
	glPopMatrix();

}

void mejaMakan(void){
	// lingkaran meja
	glPushMatrix();
	glColor3ub(46, 79, 219);
	glTranslatef(0, 1.5, -1);
	glScaled(1, 0.1, 1);
	glutSolidSphere(6, 25, 25);
	glPopMatrix();
	// kaki meja kiri depan
	glPushMatrix();
	glScaled(0.1, 0.6, 0.1);
	glTranslatef(33, 0, 30);
	glColor3ub(46, 79, 219);
	glutSolidCube(5.0);
	glPopMatrix();
	// kaki meja kiri belakang
	glPushMatrix();
	glScaled(0.1, 0.6, 0.1);
	glTranslatef(-33, 0, 30);
	glColor3ub(46, 79, 219);
	glutSolidCube(5.0);
	glPopMatrix();
	// kaki meja kanan depan
	glPushMatrix();
	glScaled(0.1, 0.6, 0.1);
	glTranslatef(33, 0, -50);
	glColor3ub(46, 79, 219);
	glutSolidCube(5.0);
	glPopMatrix();
	// kaki meja kanan belakang
	glPushMatrix();
	glScaled(0.1, 0.6, 0.1);
	glTranslatef(-33, 0, -50);
	glColor3ub(46, 79, 219);
	glutSolidCube(5.0);
	glPopMatrix();
}

void kursiMakan(void)
{
	 glPushMatrix();
		glColor3ub(46, 79, 219);
		glTranslatef(-5, -9.1, -0.05);
		glScaled(0.2, 0.05, 0.2);
		glutSolidSphere(6, 25, 25);
	 glPopMatrix();

     glPushMatrix();
        glTranslatef(-5.0,-7.0,0);
        glScalef(3.0,0.1,3.0);
        glutSolidCube(1.0);
     glPopMatrix();

     glPushMatrix();
        glTranslatef(-6.5,-5.5,0);
        glRotatef(90,0.0,0.0,1.0);
        glScalef(3.0,0.1,3.0);
        glutSolidCube(1.0);
     glPopMatrix();

     glPushMatrix();
        glTranslatef(-5.0,-8.0,0);
        glScalef(0.5,2.0,0.5);
        glutSolidCube(1.0);
     glPopMatrix();
}

void pagar(){
    glPushMatrix();
    glScaled(0.2, 1 ,0.2);
    glTranslatef(20, 0, 290);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();

    glPushMatrix();
    glScaled(0.2, 1 ,0.2);
    glTranslatef(70, 0, 290);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();

    glPushMatrix();
    glScaled(0.2, 1 ,0.2);
    glTranslatef(120, 0, 290);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();

    glPushMatrix();
    glScaled(0.2, 1 ,0.2);
    glTranslatef(170, 0, 290);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();

    glPushMatrix();
    glScaled(0.2, 1 ,0.2);
    glTranslatef(220, 0, 290);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();

    glPushMatrix();
    glScaled(0.2, 1 ,0.2);
    glTranslatef(270, 0, 290);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();

    glPushMatrix();
    glScaled(0.2, 1 ,0.2);
    glTranslatef(320, 0, 290);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glColor3f(1.0000, 0.5252, 0.0157);
    glutSolidCube(5.0);
    glPopMatrix();


    // papan memanjang
		glPushMatrix();
		glScaled(12, 0.2 ,0.05);
		glTranslatef(2.8, 5, 1160);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0000, 0.5252, 0.0157);
		glutSolidCube(5.0);
		glPopMatrix();

		glPushMatrix();
		glScaled(12, 0.2 ,0.05);
		glTranslatef(2.8, -5, 1160);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3f(1.0000, 0.5252, 0.0157);
		glutSolidCube(5.0);
		glPopMatrix();

}

void pohonCemara(){
	//batang
	glColor3f(0.8, 0.5, 0.2);
	glPushMatrix();
	glScalef(0.2, 2, 0.2);
	glutSolidSphere(1.0, 20, 16);
	glPopMatrix();
	//daun
	glColor3f(0.0, 1.0, 0.0);
	glPushMatrix();
	glScalef(1, 1, 1.0);
	glTranslatef(0, 1, 0);
	glRotatef(270, 1, 0, 0);
	glutSolidCone(1,3,10,1);
	glPopMatrix();
	glPushMatrix();
	glScalef(1, 1, 1.0);
	glTranslatef(0, 2, 0);
	glRotatef(270, 1, 0, 0);
	glutSolidCone(1,3,10,1);
	glPopMatrix();
	glPushMatrix();
	glScalef(1, 1, 1.0);
	glTranslatef(0, 3, 0);
	glRotatef(270, 1, 0, 0);
	glutSolidCone(1,3,10,1);
	glPopMatrix();
}

void TepiKolam(){
		//tepi kiri
		glPushMatrix();
		glScaled(0.1, 0.02, 1.65);
		glTranslatef(45, 0, 0.2);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glColor3ub(255, 255, 255);
		glutSolidCube(5.0);
		glPopMatrix();
		// tepi kanan
		glPushMatrix();
		glScaled(0.1, 0.02, 1.65);
		glTranslatef(80, 0, 0.2);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glutSolidCube(5.0);
		glPopMatrix();
		// tepi depan
		glPushMatrix();
		glScaled(0.8, 0.02, 0.1);
		glTranslatef(7.8, 0, 42);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
		glutSolidCube(5.0);
		glPopMatrix();
		// tepi belakang
		glPushMatrix();
		glScaled(0.8, 0.02, 0.1);
		glTranslatef(7.8, 0, -35.5);
		glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

		glutSolidCube(5.0);
		glPopMatrix();

}

unsigned int LoadTextureFromBmpFile(char *filename);

void display(void) {
	glClearStencil(0); //clear the stencil buffer
	glClearDepth(1.0f);//Fungsi menentukan nilai yang jelas untuk kedalaman
	glClearColor(0.0, 0.6, 0.8, 1);//mendefinisikan warna dari windows
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT); //untuk membersihkan memori buffer warna atau memori buffer kedalaman dari keadaan sebelumnya.
	glLoadIdentity();//untuk memanggil matriks terakhir yang disimpan.
	gluLookAt(camx, camy, camz, 0.0, 0.0, 5.0, 0.0, 1.0, 0.0);//Fungsi mendefinisikan transformasi pandangan.

		glPushMatrix();
			glTranslatef(0,0,0);
			glScalef(15, 15, 15);
			bangunan(); //Bangunan
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0,0,0);
			pintuUtama();//2 Pintu utama
		glPopMatrix();

		glPushMatrix();
			glTranslatef(0,0,-5);
			tangga();// Tangga lantai 1 dan 2
		glPopMatrix();

		glPushMatrix();
		    glTranslatef(0, 0.5, -20);
		    kursi();// Kursi
		glPopMatrix();

		glPushMatrix();
			glScalef(2.5, 2.5, 2.5);
			glTranslatef(3, 2, -10);
			mejaMakan();// Meja makan
		glPopMatrix();

		glPushMatrix();
			glScalef(2.5, 2.5, 2.5);
		    glRotatef(90, 0, 1, 0);
		    glTranslatef(10, 9.5, 3);
		    kursiMakan();// Kursi meja makan

			glPushMatrix();
			glRotatef(-180, 0, 1, 0);
			glTranslatef(-2, 0, 0);
			kursiMakan();
			glPopMatrix();

			glPushMatrix();
			glRotatef(-90, 0, 1, 0);
			glTranslatef(-2, 0, -1);
			kursiMakan();
			glPopMatrix();

			glPushMatrix();
			glRotatef(90, 0, 1, 0);
			glTranslatef(-1, 0, 0.6);
			kursiMakan();
			glPopMatrix();
		glPopMatrix();

		glPushMatrix();
			glScalef(2.5, 2.5, 2.5);
			glRotatef(90, 0, 1, 0);
			glTranslatef(-1.6, 24.7, 5);
			tempatTidur();// Tempat tidur
		glPopMatrix();

		glPushMatrix();
			glRotatef(90, 0, 1, 0);
			glTranslatef(-22.5, 15, 5);
			glScalef(1.5, 1.5, 1.5);
			meja();// Meja ruang tamu
		glPopMatrix();

		glPushMatrix();
			glTranslatef(5, 13, 35);
			glScalef(1.3, 1.2, 1);
			sofaPanjang();// Sofa panjang ruang tamu
			glPushMatrix();
				glRotatef(90, 0, 1, 0);
				glTranslatef(0, 0, 25);
				sofaPanjang();
			glPopMatrix();
		glPopMatrix();

		glPushMatrix();
			glRotatef(-90, 0, 1, 0);
			glTranslatef(22.5, 13, 15);
			glScalef(0.5, 1.2, 1);
			sofaPanjang();//Sofa panjang ruang tamu

			glPushMatrix();
				glRotatef(90, 0, 1, 0);
				glTranslatef(0, 0, 40);
				sofaPanjang();
			glPopMatrix();

			glPushMatrix();
				glRotatef(-90, 0, 1, 0);
				glTranslatef(-70, 30, 12);
				sofaPanjang();
			glPopMatrix();
		glPopMatrix();

		glPushMatrix();
			//lampu kiri
			glScalef(0.3, 0.3, 0.3);
			glTranslatef(-75, 155, 216);
			lampuMeja(); //Lampu ruang tidur
			// lampu kanan
			glPushMatrix();
			glTranslatef(0, 0, -156);
			lampuMeja();// Lampu ruang tidur
			glPopMatrix();
		glPopMatrix();

		glPushMatrix();
			glScalef(2.5, 2.5, 2.5);
			glTranslatef(3, 2, -10);
			pagar();// pagar

			glPushMatrix();
				glTranslatef(-75, 0, 0);
				pagar();// pagar
			glPopMatrix();

			glPushMatrix(); // pagar sisi kiri
				glRotatef(-90, 0, 1, 0);
				glTranslatef(-7, 0, 13);
				pagar();// pagar
				glPushMatrix();
				glTranslatef(-30, 0, 0);
				pagar();// pagar
				glPopMatrix();
			glPopMatrix();

			glPushMatrix(); // pagar sisi kanan
				glRotatef(-90, 0, 1, 0);
				glTranslatef(-7, 0, -122);
				pagar();// pagar
				glPushMatrix();
				glTranslatef(-30, 0, 0);
				pagar();// pagar
				glPopMatrix();
			glPopMatrix();

			glPushMatrix(); // pagar sisi belakang
				glTranslatef(0, 0, -91);
				pagar();// pagar
				glPushMatrix();
				glTranslatef(-60, 0, 0);
				pagar();// pagar
				glPopMatrix();
				glPushMatrix();
				glTranslatef(-70, 0, 0);
				pagar();// pagar
				glPopMatrix();
			glPopMatrix();
		glPopMatrix();

		// POHON CEMARA
		glPushMatrix();
		glColor3f(1, 0, 0);
		glTranslatef(-110, 0, 85);
		glScalef(10, 10, 10);
		pohonCemara();
		glPopMatrix();

		glPushMatrix();
		glColor3f(1, 0, 0);
		glTranslatef(140, 0, 75);
		glScalef(10, 10, 10);
		pohonCemara();
		glPopMatrix();


		glPushMatrix();
		glColor3f(1, 0, 0);
		glTranslatef(110, 0, -80);
		glScalef(10, 10, 10);
		pohonCemara();
		glPopMatrix();

		glPushMatrix();
		glColor3f(1, 0, 0);
		glTranslatef(-80, 0, -40);
		glScalef(17, 14, 17);
		pohonCemara();
		glPopMatrix();

		glPushMatrix();
		glColor3f(1, 0, 0);
		glTranslatef(150, 0, 50);
		glScalef(17, 14, 17);
		pohonCemara();
		glPopMatrix();

	    //Kolam
	    glEnable(GL_TEXTURE_2D);
	    glColor3ub(000,191,255);
	    load_BMP_texture("water.bmp");
	    glBegin(GL_POLYGON);
	                        glTexCoord2f(0,0);
	                        glVertex3f(120, -1, -50);
	                        glTexCoord2f(1,0);
	                        glVertex3f(70, -1, -50);
	                        glTexCoord2f(1,1);
	                        glVertex3f(70, -1, 60);
	                        glTexCoord2f(0,1);
	                        glVertex3f(120, -1, 60);
	    glEnd();
	    glDisable(GL_TEXTURE_2D);

	    // Tepi kolam
		glPushMatrix();
			glTranslatef(0,0,0);
			glScalef(15, 15, 15);
			TepiKolam(); //Bangunan
		glPopMatrix();

	glPushMatrix();
	displayRumput(_Rumput, 0.3f, 0.9f, 0.0f);// Memanggil class rumpunt dan inisialisasi warna
	glPopMatrix();
	glutSwapBuffers();
	glFlush();
	rot++;
	angle++;

}

void init(void) {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glDepthFunc(GL_LESS);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_SMOOTH);
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);//Fungsi set up perspektif matriks proyeksi.
	glEnable(GL_CULL_FACE);

	_Rumput = loadRumput("heightmap.bmp", -20);
	_RumputTanah = loadRumput("heightmapTanah.bmp", -20);
	_RumputAir = loadRumput("heightmapAir.bmp", -20);
}

static void navigasi(int key, int x, int y) {
	switch (key) {
	case GLUT_KEY_HOME:
		camy++;
		break;
	case GLUT_KEY_END:
		camy--;
		break;
	case GLUT_KEY_UP:
		camz--;
		break;
	case GLUT_KEY_DOWN:
		camz++;
		break;
	case GLUT_KEY_RIGHT:
		camx++;
		break;
	case GLUT_KEY_LEFT:
		camx--;
		break;

	case GLUT_KEY_F1: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);//Yaitu menyebabkan warna sekitar/ pantulan yang telah banyak.
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);//Yaitu cahaya yang diterima oleh material akan menyebar kesegala arah.
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	case GLUT_KEY_F2: {
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient2);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse2);
		glMaterialfv(GL_FRONT, GL_AMBIENT, mat_ambient);
		glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
	}
		;
		break;
	default:
		break;
	}
}

void keyboard(unsigned char key, int x, int y) {
	if (key == 'd') {

		spin = spin - 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'a') {
		spin = spin + 1;
		if (spin > 360.0)
			spin = spin - 360.0;
	}
	if (key == 'q') {
		camz++;
	}
	if (key == 'e') {
		camz--;
	}
	if (key == 's') {
		camy--;
	}
	if (key == 'w') {
		camy++;
	}
}

void reshape(int w, int h) {//Fungsi set up perspektif matriks proyeksi.
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60, (GLfloat) w / (GLfloat) h, 0.1, 1000.0);
	glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_STENCIL | GLUT_DEPTH); //add a stencil buffer to the window
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("PEMODELAN RUANG 3D KELOMPOK 7 IF10");
	init();

	glutDisplayFunc(display);
	glutIdleFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(navigasi);

	glutKeyboardFunc(keyboard);

	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);//mengatur cahaya kilau pada objek
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);// mengkonfigurasi sumber cahaya

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);///mengatur cahaya kilau pada objek
	glMaterialfv(GL_FRONT, GL_SHININESS, high_shininess);// mengatur definisi kesilauan
	glColorMaterial(GL_FRONT, GL_DIFFUSE);

	glutMainLoop();
	return 0;
}
