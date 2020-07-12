#define _USE_MATH_DEFINES

#include <stdio.h>
#include <stdlib.h>
#include "tinyxml2/tinyxml2.cpp"
#include "tinyxml2/tinyxml2.h"
#include <GL/glew.h>
#include <GL/glut.h>
#include <math.h>
#include <vector>
#include <iostream> 

using namespace tinyxml2;
using namespace std;

 struct Triangle{  //ja nao � usado na estrutura de baixo, mas fica mais limpo no scanf no parsefiles, por isso pode ficar�para j�.
	float x1, y1, z1;// se quizerem fa�am vars no parsefiles e podem tirar
	float x2, y2, z2;
	float x3, y3, z3;
};

 struct Point {  
	 float x, y, z;
 };

struct Model {
	float tx = 0, ty = 0, tz =0;
	float transtime = 0;

	float ra=0, rx=0, ry=0, rz=0;
	float rotatetime = 0;

	float sx = 1, sy = 1, sz = 1;

	float up[3] = { 0,1,0};
	vector<Point> catPoints;
	float p[1024][3];
	int pcount = 0;


	string nome;
	int pos; //posi�ao dos pontos(triangulos) no arreio pontos
	vector<Model> submodules;
};

vector<Model> planetas; //main struct
vector<vector<float>> pontos; //so os pontos em si
GLuint buffers[64]; //buffers para as vbo's

bool axis = false; //para desenhar os eixos, da jeito para debug, tecla X

float camX = 140, camY = 80, camZ = 140; //camara
int startX, startY, tracking = 0, alpha = 0, beta = 25, r = 200; //camara

int frame = 0, time = 0, timebase = 0, fps = 0; 

void changeSize(int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window with zero width).
	if(h == 0)
		h = 1;

	// compute window's aspect ratio 
	float ratio = w * 1.0 / h;

	// Set the projection matrix as current
	glMatrixMode(GL_PROJECTION);
	// Load Identity Matrix
	glLoadIdentity();

// Set the viewport to be the entire window
glViewport(0, 0, w, h);

// Set perspective
gluPerspective(45.0f, ratio, 1.0f, 1000.0f);

// return to the model view matrix mode
glMatrixMode(GL_MODELVIEW);
}



//rato e teclado
void processMouseButtons(int button, int state, int xx, int yy)
{
	if (state == GLUT_DOWN) {
		startX = xx;
		startY = yy;
		if (button == GLUT_LEFT_BUTTON)
			tracking = 1;
		else if (button == GLUT_RIGHT_BUTTON)
			tracking = 2;
		else
			tracking = 0;
	}
	else if (state == GLUT_UP) {
		if (tracking == 1) {
			alpha += (xx - startX);
			beta += (yy - startY);
		}
		else if (tracking == 2) {

			r -= yy - startY;
			if (r < 3)
				r = 3.0;
		}
		tracking = 0;
	}

}

void processMouseMotion(int xx, int yy)
{
	int deltaX, deltaY;
	int alphaAux, betaAux;
	int rAux;

	if (!tracking)
		return;

	deltaX = xx - startX;
	deltaY = yy - startY;

	if (tracking == 1) {

		alphaAux = alpha + deltaX;
		betaAux = beta + deltaY;

		if (betaAux > 85.0)
			betaAux = 85.0;
		else if (betaAux < -85.0)
			betaAux = -85.0;

		rAux = r;
	}
	else if (tracking == 2) {

		alphaAux = alpha;
		betaAux = beta;
		rAux = r - deltaY;
		if (rAux < 3)
			rAux = 3;
	}
	camX = rAux * sin(alphaAux * 3.14 / 180.0) * cos(betaAux * 3.14 / 180.0);
	camZ = rAux * cos(alphaAux * 3.14 / 180.0) * cos(betaAux * 3.14 / 180.0);
	camY = rAux * sin(betaAux * 3.14 / 180.0);


}

void processKeys(unsigned char c, int xx, int yy) {

	switch (c) {

	case 'd':
		break;
	
	case 'x':
		axis = !axis;
		break;
}

}



//itera os ficheiros de dados .3d e guarda numa estrutura de dados
void parseFiles(vector<Model>& planetas) {
	for (int r = 0; r < planetas.size(); r++) {
		FILE* file = fopen(planetas.at(r).nome.data(), "r");
		char* buffer = (char*)malloc(sizeof(char) * 1024);
		pontos.push_back({});  //cria uma novo vector vazio
		int location = pontos.size(); //talvesz tenha que ser mais 1 no primeiro, offeset
		while (fgets(buffer, 1024, file)) {
			Triangle tri;
			int shutup = sscanf(buffer, "%f%f%f%f%f%f%f%f%f", &tri.x1, &tri.y1, &tri.z1, &tri.x2, &tri.y2, &tri.z2, &tri.x3, &tri.y3, &tri.z3);
			pontos.at(location-1).insert(pontos.at(location-1).end(), { tri.x1, tri.y1, tri.z1, tri.x2, tri.y2, tri.z2, tri.x3, tri.y3, tri.z3 });
		}
		planetas.at(r).pos = location-1;
		fclose(file);
		free(buffer);
		if (planetas.at(r).submodules.size() > 0) { parseFiles(planetas.at(r).submodules); } //recursividade, para dar load aos submodules
	}

};

//da parse ao config.xml e cria a esturtura de dados recursivamente, mas ainda n�o le os ficheiros.3d, isso � na parsefiles
void parseConfig(XMLElement* grupo, vector<Model> & planetass) {  
	const char* nome, * tx, * ty, * tz, * ra, * rx, * ry, * rz, * sx, * sy, * sz, * ttime, * rtime;
	while (grupo) {   
		Model planet;

		XMLElement* trans = grupo->FirstChildElement("translate");
		if (trans) {
			tx = trans->Attribute("X"); ty = trans->Attribute("Y"); tz = trans->Attribute("Z");
			if (tx) { planet.tx = atof(tx); } if (ty) { planet.ty = atof(ty); } if (tz) { planet.tz = atof(tz); }

			ttime = trans->Attribute("time");
			if (ttime) {
				planet.transtime = atof(ttime);
				XMLElement* point = trans -> FirstChildElement("point");
				while (point) {
					Point asd;
					asd.x = atof(point->Attribute("X"));
					asd.y = atof(point->Attribute("Y"));
					asd.z = atof(point->Attribute("Z"));
					planet.catPoints.push_back(asd);
					planet.p[planet.pcount][0] = atof(point->Attribute("X"));;
					planet.p[planet.pcount][1] = atof(point->Attribute("Y"));;
					planet.p[planet.pcount][2] = atof(point->Attribute("Z"));;
					planet.pcount++;
					point = point->NextSiblingElement();
				}
			}
		}

		XMLElement* scale = grupo->FirstChildElement("scale");
		if (scale) {
			sx = scale->Attribute("X"); sy = scale->Attribute("Y"); sz = scale->Attribute("Z");
			if (sx) { planet.sx = atof(sx); }if (sy) { planet.sy = atof(sy); } if (sz) { planet.sz = atof(sz); }
		}

		XMLElement* rotate = grupo->FirstChildElement("rotate");
		if (rotate) {
			rx = rotate->Attribute("axisX"); ry = rotate->Attribute("axisY"); rz = rotate->Attribute("axisZ"); ra = rotate->Attribute("angle");
			if (rx) { planet.rx = atof(rx); }if (ry) { planet.ry = atof(ry); } if (rz) { planet.rz = atof(rz); } if (ra) { planet.ra = atof(ra);}
			rtime = rotate->Attribute("time");
			if (rtime) { planet.rotatetime = atof(rtime); }
		}

		XMLElement* models = grupo->FirstChildElement("models");
		if (models) {
			XMLElement* model = models->FirstChildElement("model");
			nome = model->Attribute("file");
		}

		XMLElement* submodels = grupo->FirstChildElement("group");
		if (submodels) {
			parseConfig(submodels, planet.submodules);//recursividade, assim le os submodels como se fosse um model, vai dar ao mesmo
		}

		planet.nome = nome;
		planetass.push_back(planet);
		grupo = grupo->NextSiblingElement();
	}
}





void buildRotMatrix(float* x, float* y, float* z, float* m) {
	m[0] = x[0]; m[1] = x[1]; m[2] = x[2]; m[3] = 0;
	m[4] = y[0]; m[5] = y[1]; m[6] = y[2]; m[7] = 0;
	m[8] = z[0]; m[9] = z[1]; m[10] = z[2]; m[11] = 0;
	m[12] = 0; m[13] = 0; m[14] = 0; m[15] = 1;
}

void cross(float* a, float* b, float* res) {
	res[0] = a[1] * b[2] - a[2] * b[1];
	res[1] = a[2] * b[0] - a[0] * b[2];
	res[2] = a[0] * b[1] - a[1] * b[0];
}

void normalize(float* a) {
	float l = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
	a[0] = a[0] / l;
	a[1] = a[1] / l;
	a[2] = a[2] / l;
}

void multMatrixVector(float* m, float* v, float* res) {

	for (int j = 0; j < 4; ++j) {
		res[j] = 0;
		for (int k = 0; k < 4; ++k) {
			res[j] += v[k] * m[j * 4 + k];
		}
	}

}

void transpose(float A[][4], float B[][4])
{
	int i, j;
	for (i = 0; i < 4; i++)
		for (j = 0; j < 4; j++)
			B[i][j] = A[j][i];

	int asd = 3;
}

void getCatmullRomPoint(float t, float* p0, float* p1, float* p2, float* p3, float* pos, float* deriv) {

	// catmull-rom matrix
	float m[4][4] = { {-0.5f,  1.5f, -1.5f,  0.5f},
						{ 1.0f, -2.5f,  2.0f, -0.5f},
						{-0.5f,  0.0f,  0.5f,  0.0f},
						{ 0.0f,  1.0f,  0.0f,  0.0f} };


	float xx[4] = { p0[0],p1[0],p2[0],p3[0] };
	float yy[4] = { p0[1],p1[1],p2[1],p3[1] };
	float zz[4] = { p0[2],p1[2],p2[2],p3[2] };

	// Compute A=M*P
	float ax[4];
	float ay[4];
	float az[4];

	multMatrixVector((float*)m, (float*)xx, (float*)ax);
	multMatrixVector((float*)m, (float*)yy, (float*)ay);
	multMatrixVector((float*)m, (float*)zz, (float*)az);
	// Compute pos=T*A

	float T[4] = { pow(t,3), pow(t,2), t, 1 };
	pos[0] = T[0] * ax[0] + T[1] * ax[1] + T[2] * ax[2] + T[3] * ax[3];
	pos[1] = T[0] * ay[0] + T[1] * ay[1] + T[2] * ay[2] + T[3] * ay[3];
	pos[2] = T[0] * az[0] + T[1] * az[1] + T[2] * az[2] + T[3] * az[3];

	// compute deriv=T'*A
	float TT[4] = { 3 * pow(t,2), 2 * pow(t,1), 1, 0 };
	deriv[0] = TT[0] * ax[0] + TT[1] * ax[1] + TT[2] * ax[2] + TT[3] * ax[3];
	deriv[1] = TT[0] * ay[0] + TT[1] * ay[1] + TT[2] * ay[2] + TT[3] * ay[3];
	deriv[2] = TT[0] * az[0] + TT[1] * az[1] + TT[2] * az[2] + TT[3] * az[3];
}

void getGlobalCatmullRomPoint(float gt, float* pos, float* deriv, Model m) {

	int POINT_COUNT = m.catPoints.size();
	float t = gt * POINT_COUNT; // this is the real global t
	int index = floor(t);  // which segment
	t = t - index; // where within  the segment

	// indices store the points
	int indices[4];
	indices[0] = (index + POINT_COUNT - 1) % POINT_COUNT;
	indices[1] = (indices[0] + 1) % POINT_COUNT;
	indices[2] = (indices[1] + 1) % POINT_COUNT;
	indices[3] = (indices[2] + 1) % POINT_COUNT;

	getCatmullRomPoint(t, m.p[indices[0]], m.p[indices[1]], m.p[indices[2]], m.p[indices[3]], pos, deriv);
}

void renderCatmullRomCurve(Model m) {

	float pos[3];
	float deriv[3];
	// desenhar a curva usando segmentos de reta-GL_LINE_LOOP
	glBegin(GL_LINE_LOOP);
	for (float i = 0.0; i < 1.0; i += 0.01) {
		getGlobalCatmullRomPoint(i, (float*)pos, (float*)deriv,  m);
		glVertex3f(pos[0], pos[1], pos[2]);
	}
	glEnd();
}




//bind e bufferdata para vbo's
void createVBO() {

	glGenBuffers(pontos.size(),buffers);

	for (int a = 0; a < pontos.size(); a++) {
		glBindBuffer(GL_ARRAY_BUFFER, buffers[a]);
		glBufferData(GL_ARRAY_BUFFER, pontos.at(a).size() * sizeof(float), &pontos.at(a).front(), GL_STATIC_DRAW);
	}
}

//desenha VBO's
void drawVBO(vector<Model>& planetas) {
	
	float pos[3];
	float derivada[3];
	float z[3];
	float m[4][4];
	float mm[4][4];

	for (int p = 0; p < planetas.size(); p++) {
		glPushMatrix();
		float a = glutGet(GLUT_ELAPSED_TIME);
		float tt = fmod((a / (planetas.at(p).transtime * 1000)), 1); //catmull
		float ttt = fmod((a / (planetas.at(p).transtime * 1000)), 1) * 360; //rota�ao em torno da estrela
		float tttt = fmod((a / (planetas.at(p).rotatetime * 1000)), 1) * 360;//rota�ao em torno de si mesmo

		if (planetas.at(p).transtime != 0 && planetas.at(p).catPoints.size() > 0)  { //calcular o ponton catmull, translate e rotate para a posi�ao certa
			renderCatmullRomCurve(planetas.at(p));
			getGlobalCatmullRomPoint(tt, (float*)pos, (float*)derivada, planetas.at(p));
			glTranslatef(pos[0], pos[1], pos[2]);
			if(planetas.at(p).nome == "bezier.3d") {
				glRotatef(180, 1, 0, 0);///////////////////////fix para ficar no eixo dos y???????
			}
			normalize((float*)derivada);

			cross((float*)derivada, (float*)planetas.at(p).up, (float*)z);
			normalize((float*)z);

			cross((float*)z, (float*)derivada, (float*)planetas.at(p).up);
			normalize((float*)planetas.at(p).up);

			buildRotMatrix(derivada, planetas.at(p).up, z, *m);

			transpose(m, mm); //nao faz nada?! sao sempre simetricas

			glMultMatrixf((float*)*mm);
		}
		
		else if (planetas.at(p).transtime != 0) { //se nao tiver catmull � uma rota�ao estatica em torno do eixo selecionado
			glRotatef(ttt, planetas.at(p).rx, planetas.at(p).ry, planetas.at(p).rz);
			glTranslatef(planetas.at(p).tx, planetas.at(p).ty, planetas.at(p).tz);
				}


		if (planetas.at(p).rotatetime != 0) {
			glRotatef(tttt, planetas.at(p).rx, planetas.at(p).ry, planetas.at(p).rz); //rota�ao port tempo
		}
		else if (planetas.at(p).ra!=0) {
			glRotatef(planetas.at(p).ra, planetas.at(p).rx, planetas.at(p).ry, planetas.at(p).rz); //rota�ao estatica
		}



		glScalef(planetas.at(p).sx, planetas.at(p).sy, planetas.at(p).sz);
		glColor3f(1.0f, 1.0f, 1.0f);

		int location = planetas.at(p).pos;
		
		glBindBuffer(GL_ARRAY_BUFFER, buffers[location]);
		glVertexPointer(3, GL_FLOAT, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, pontos.at(location).size() / 3);

		if (planetas.at(p).submodules.size() > 0) { 
			drawVBO(planetas.at(p).submodules);//recursividade para os submodels
		} 

		glPopMatrix();
		
	}


	
}

void drawAxis() {

	if (axis) { //eixos
		glBegin(GL_LINES);
		// X axis in red
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(1000.0f, 0.0f, 0.0f);
		// Y Axis in Green
		glColor3f(0.0f, 1.0f, 0.0f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(0.0f, 1000.0f, 0.0f);
		// Z Axis in Blue
		glColor3f(0.0f, 0.0f, 1.0f);
		glVertex3f(0.0f, 0.0f, -0.00f);
		glVertex3f(0.0f, 0.0f, 1000.0f);
		glEnd();
	}
}





void renderScene(void) {

	// clear buffers
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	frame++;
	time = glutGet(GLUT_ELAPSED_TIME);
	if (time - timebase > 1000) {
		fps = frame * 1000.0 / (time - timebase);
		timebase = time;
		frame = 0;
	}
	string s = "d";
	sprintf(&s.front(), "%d", fps);
	glutSetWindowTitle(s.data());

	// set the camera
	glLoadIdentity();
	gluLookAt(camX, camY, camZ,
		0.0, 0.0, 0.0,
		0.0f, 1.0f, 0.0f);

	drawAxis();
	drawVBO(planetas);
	// End of frame
	glutSwapBuffers();
	
}

int main(int argc, char** argv) {
	// init GLUT and the window
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE|GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(800,800);
	glutCreateWindow("CG@DI-UM");
		
// Required callback registry 
	glutDisplayFunc(renderScene);
	glutIdleFunc(renderScene);
	glutReshapeFunc(changeSize);
	
// Callback registration for keyboard processing
	glutKeyboardFunc(processKeys);
	glutMouseFunc(processMouseButtons);
	glutMotionFunc(processMouseMotion);
	glewInit();
	glEnableClientState(GL_VERTEX_ARRAY);

	//glutIdleFunc(renderScene);
//  OpenGL settings
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glPolygonMode(GL_FRONT, GL_LINE);

// enter GLUT's main cycle

	printf("Movimento com o rato, botao esquerdo move, botao direito zoom");
	
	XMLDocument doc;
	doc.LoadFile("config.xml");
	XMLElement* cena = doc.FirstChildElement();
	XMLElement* grupo = cena->FirstChildElement("group");
	parseConfig(grupo, planetas);
	parseFiles(planetas);  //itera os ficheiros de dados e guarda numa estrutura de dados
	createVBO();
	glutMainLoop();
	
	return 1;
}
