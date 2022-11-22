////////////////////////////////////////////////////////////////////////////////
//
// File: virtualLego.cpp
//
// Original Author: ¹ÚÃ¢Çö Chang-hyeon Park, 
// Modified by Bong-Soo Sohn and Dong-Jun Kim
// 
// Originally programmed for Virtual LEGO. 
// Modified later to program for Virtual Billiard.
//        
////////////////////////////////////////////////////////////////////////////////

#include "d3dUtility.h"
#include <vector>
#include <ctime>
#include <cstdlib>
#include <cstdio>
#include <cassert>
#include <cmath>

IDirect3DDevice9* Device = NULL;

// window size
const int Width  = 1024;
const int Height = 768;

// There are 20 balls
// initialize the position (coordinate) of each ball
const int totalBalls = 20;
const float spherePos[totalBalls][2] = {
	{-2.f, 4.0f} , {-1.5f,4.0f} , {0.0f,4.0f} , {1.5f,4.0f}, {2.0f,4.0f},
	{-2.f,3.0f} , {-1.0f,3.0f} , {0.0f,3.0f} , {1.0f,3.0f}, {2.0f,3.0f},
	{-2.f,2.0f} , {-1.0f,2.0f} , {0.0f,2.0f} , {1.0f,2.0f}, {2.0f,2.0f},
	{-2.f,1.0f} , {-1.5f,1.0f} , {0.0f,1.0f} , {1.5f,1.0f}, {2.0f,1.0f},
}; 
// initialize the color of each ball
const D3DXCOLOR ballColor = d3d::YELLOW;

// -----------------------------------------------------------------------------
// Transform matrices
// -----------------------------------------------------------------------------
D3DXMATRIX g_mWorld;
D3DXMATRIX g_mView;
D3DXMATRIX g_mProj;

#define M_RADIUS 0.21   // ball radius
#define PI 3.14159265
#define M_HEIGHT 0.01
#define DECREASE_RATE 0.9982

#define KEYSTEP 0.1
#define REDBALLSPEED 30.0
#define MAXSCORE 200

// global constants
const float initialGreyBallPosZ = -4.0f + (float)M_RADIUS + 0.05f;
const float initialRedBallPosZ = initialGreyBallPosZ + (float)M_RADIUS * 2 + 0.05f;
const float horizontalBarWidth = 6.0f;
const float verticalBarDepth = 9.0f;
const float wallThickness = 0.12f;
int life = 5;
int score = 0;
bool isRoundStarted = false;
bool isGameEnded = false;

// -----------------------------------------------------------------------------
// CSphere class definition
// -----------------------------------------------------------------------------

class CSphere {
private :
	float					center_x, center_y, center_z;
    float                   m_radius;
	float					m_velocity_x;
	float					m_velocity_z;

public:
    CSphere(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_radius = 0;
		m_velocity_x = 0;
		m_velocity_z = 0;
        m_pSphereMesh = NULL;
    }
    ~CSphere(void) {}

public:
    bool create(IDirect3DDevice9* pDevice, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;
		
        m_mtrl.Ambient  = color;
        m_mtrl.Diffuse  = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power    = 5.0f;
		
        if (FAILED(D3DXCreateSphere(pDevice, getRadius(), 50, 50, &m_pSphereMesh, NULL)))
            return false;
        return true;
    }
	
    void destroy(void)
    {
        if (m_pSphereMesh != NULL) {
            m_pSphereMesh->Release();
            m_pSphereMesh = NULL;
        }
    }

    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
		m_pSphereMesh->DrawSubset(0);
    }
	
    bool hasIntersected(CSphere& ball) 
	{
		// Insert your code here.
		if (originDistanceFrom(ball) <= 0.42) {
			return true;
		}
		else {
			return false;
		}
	}
	
	void hitBy(CSphere& ball) 
	{ 
		// Insert your code here.
		if (hasIntersected(ball)) {
			// I think that if one of the ball intersected is yellow, then distroy that and save the grey ball.
			float vXAfterCollision = (ball.getVelocity_X() + m_velocity_x) * ball.getVelocity_X()/ball.getVelocity_Z();
			float vZAfterCollision = (ball.getVelocity_Z() + m_velocity_z);
			if ((D3DXCOLOR)m_mtrl.Ambient == (D3DXCOLOR)d3d::YELLOW) {
				ball.setPower(-vXAfterCollision, -vZAfterCollision);
				this->setCenter(center_x, -500.0f, center_z);
				score += 10;
			}
			else if ((D3DXCOLOR)ball.m_mtrl.Ambient == (D3DXCOLOR)d3d::YELLOW) {
				this->setPower(-vXAfterCollision, -vZAfterCollision);
				ball.setCenter(center_x, -500.0f, center_z);
				score += 10;
			}
			// else, it is collision of grey and red balls.
			else {
				if ((D3DXCOLOR)ball.m_mtrl.Ambient == (D3DXCOLOR)d3d::RED) {
					ball.setPower(ball.m_velocity_x, -1 * ball.m_velocity_z);
				}
			}
			if (score >= (int)MAXSCORE) {
				ball.setPower(0.0, 0.0);
				isRoundStarted = false;
				isGameEnded = true;
			}
		}
	}

	void ballUpdate(float timeDiff) 
	{
		const float TIME_SCALE = 3.3;
		D3DXVECTOR3 cord = this->getCenter();
		double vx = abs(this->getVelocity_X());
		double vz = abs(this->getVelocity_Z());

		if(vx > 0.01 || vz > 0.01)
		{
			float tX = cord.x + TIME_SCALE*timeDiff*m_velocity_x;
			float tZ = cord.z + TIME_SCALE*timeDiff*m_velocity_z;

			//correction of position of ball
			// Please uncomment this part because this correction of ball position is necessary when a ball collides with a wall
			float xBound = horizontalBarWidth / 2 - wallThickness / 2;
			float zBound = verticalBarDepth / 2 - wallThickness / 2;
			if(tX >= (xBound - M_RADIUS))
				tX = xBound - M_RADIUS;
			else if(tX <= (-1 * xBound + M_RADIUS))
				tX = -xBound + M_RADIUS;
			else if(tZ <= (-1 * zBound + M_RADIUS))
				tZ = -zBound + M_RADIUS;
			else if(tZ >= (zBound - M_RADIUS))
				tZ = zBound - M_RADIUS;
			
			this->setCenter(tX, cord.y, tZ);
		}
		else { this->setPower(0,0);}
		double rate = 1 -  (1 - DECREASE_RATE)*timeDiff;
		if(rate < 0 )
			rate = 0;
		this->setPower(getVelocity_X(), getVelocity_Z());
	}

	double getVelocity_X() { return this->m_velocity_x;	}
	void setVelocity_X(float velocity_x) {
		this->m_velocity_x = velocity_x;
	}
	double getVelocity_Z() { return this->m_velocity_z; }
	void setVelocity_Z(float velocity_z) {
		this->m_velocity_z = velocity_z;
	}

	void setPower(double vx, double vz)
	{
		this->m_velocity_x = vx;
		this->m_velocity_z = vz;
	}

	void setCenter(float x, float y, float z)
	{
		D3DXMATRIX m;
		center_x=x;	center_y=y;	center_z=z;
		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}
	
	float getRadius(void)  const { return (float)(M_RADIUS);  }
    const D3DXMATRIX& getLocalTransform(void) const { return m_mLocal; }
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
    D3DXVECTOR3 getCenter(void) const
    {
        D3DXVECTOR3 org(center_x, center_y, center_z);
        return org;
    }

	D3DXCOLOR getColor(void) {
		return (D3DXCOLOR)m_mtrl.Ambient;
	}
	
private:
    D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh*              m_pSphereMesh;
	
	float originDistanceFrom(CSphere& ball) {
		double xSq = pow((center_x - ball.center_x), 2);
		double ySq = pow((center_y - ball.center_y), 2);
		double zSq = pow((center_z - ball.center_z), 2);
		float centerDistance = (float)sqrt((xSq + ySq + zSq));
		return centerDistance;
	};
};



// -----------------------------------------------------------------------------
// CWall class definition
// -----------------------------------------------------------------------------

class CWall {

private:
	
    float					m_x;
	float					m_z;
	float                   m_width;
    float                   m_depth;
	float					m_height;
	
public:
    CWall(void)
    {
        D3DXMatrixIdentity(&m_mLocal);
        ZeroMemory(&m_mtrl, sizeof(m_mtrl));
        m_width = 0;
        m_depth = 0;
        m_pBoundMesh = NULL;
    }
    ~CWall(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, float ix, float iz, float iwidth, float iheight, float idepth, D3DXCOLOR color = d3d::WHITE)
    {
        if (NULL == pDevice)
            return false;
		
        m_mtrl.Ambient  = color;
        m_mtrl.Diffuse  = color;
        m_mtrl.Specular = color;
        m_mtrl.Emissive = d3d::BLACK;
        m_mtrl.Power    = 5.0f;
		
        m_width = iwidth;
        m_depth = idepth;
		
        if (FAILED(D3DXCreateBox(pDevice, iwidth, iheight, idepth, &m_pBoundMesh, NULL)))
            return false;
        return true;
    }
    void destroy(void)
    {
        if (m_pBoundMesh != NULL) {
            m_pBoundMesh->Release();
            m_pBoundMesh = NULL;
        }
    }
    void draw(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return;
        pDevice->SetTransform(D3DTS_WORLD, &mWorld);
        pDevice->MultiplyTransform(D3DTS_WORLD, &m_mLocal);
        pDevice->SetMaterial(&m_mtrl);
		m_pBoundMesh->DrawSubset(0);
    }
	
	bool hasIntersected(CSphere& ball)
	{
		// Insert your code here.
		D3DXVECTOR3 ballCenter = ball.getCenter();
		if (isWide()) {
			float distance = abs(ballCenter.z - m_z);
			if (distance <= (float)(M_RADIUS + m_depth/2)) {
				return true;
			}
			else {
				return false;
			}
		}
		else if (isTall()) {
			float distance = abs(ballCenter.x - m_x);
			if (distance <= (float)(M_RADIUS + m_width/2)) {
				return true;
			}
			else {
				return false;
			}
		}
	}

	void hitBy(CSphere& ball) 
	{
		// Insert your code here.
		if (hasIntersected(ball)) {
			// reflect the ball.
			if (isWide()) {
				D3DXVECTOR3 ballCenter = ball.getCenter();
				float zBound = (float)(m_z - wallThickness); 
				ball.setVelocity_Z(-1 * ball.getVelocity_Z());
				ball.setCenter(ballCenter.x, ballCenter.y, zBound - M_RADIUS - 0.05);
			}
			else if (isTall()) {
				D3DXVECTOR3 ballCenter = ball.getCenter();
				float intersectedBallPos = m_x > 0 ? m_x - wallThickness/2 - M_RADIUS - 0.05 : m_x + wallThickness/2 + M_RADIUS + 0.05;
				ball.setVelocity_X(-1 * ball.getVelocity_X());
				ball.setCenter(intersectedBallPos, ballCenter.y, ballCenter.z);
			}
		}
	}    
	
	void setPosition(float x, float y, float z)
	{
		D3DXMATRIX m;
		this->m_x = x;
		this->m_z = z;

		D3DXMatrixTranslation(&m, x, y, z);
		setLocalTransform(m);
	}

	float getPositionX(void) const { return m_x; }
	float getPositionZ(void) const { return m_z; }
	
    float getHeight(void) const { return M_HEIGHT; }
	float getWidth(void) const { return m_width; }
	
	
	
private :
    void setLocalTransform(const D3DXMATRIX& mLocal) { m_mLocal = mLocal; }
	
	D3DXMATRIX              m_mLocal;
    D3DMATERIAL9            m_mtrl;
    ID3DXMesh*              m_pBoundMesh;

	bool isWide() {
		if (m_width > m_depth) {
			return true;
		}
		else {
			return false;
		}
	};
	bool isTall() {
		if (m_depth > m_width) {
			return true;
		}
		else {
			return false;
		}
	};
};

// -----------------------------------------------------------------------------
// CLight class definition
// -----------------------------------------------------------------------------

class CLight {
public:
    CLight(void)
    {
        static DWORD i = 0;
        m_index = i++;
        D3DXMatrixIdentity(&m_mLocal);
        ::ZeroMemory(&m_lit, sizeof(m_lit));
        m_pMesh = NULL;
        m_bound._center = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        m_bound._radius = 0.0f;
    }
    ~CLight(void) {}
public:
    bool create(IDirect3DDevice9* pDevice, const D3DLIGHT9& lit, float radius = 0.1f)
    {
        if (NULL == pDevice)
            return false;
        if (FAILED(D3DXCreateSphere(pDevice, radius, 10, 10, &m_pMesh, NULL)))
            return false;
		
        m_bound._center = lit.Position;
        m_bound._radius = radius;
		
        m_lit.Type          = lit.Type;
        m_lit.Diffuse       = lit.Diffuse;
        m_lit.Specular      = lit.Specular;
        m_lit.Ambient       = lit.Ambient;
        m_lit.Position      = lit.Position;
        m_lit.Direction     = lit.Direction;
        m_lit.Range         = lit.Range;
        m_lit.Falloff       = lit.Falloff;
        m_lit.Attenuation0  = lit.Attenuation0;
        m_lit.Attenuation1  = lit.Attenuation1;
        m_lit.Attenuation2  = lit.Attenuation2;
        m_lit.Theta         = lit.Theta;
        m_lit.Phi           = lit.Phi;
        return true;
    }
    void destroy(void)
    {
        if (m_pMesh != NULL) {
            m_pMesh->Release();
            m_pMesh = NULL;
        }
    }
    bool setLight(IDirect3DDevice9* pDevice, const D3DXMATRIX& mWorld)
    {
        if (NULL == pDevice)
            return false;
		
        D3DXVECTOR3 pos(m_bound._center);
        D3DXVec3TransformCoord(&pos, &pos, &m_mLocal);
        D3DXVec3TransformCoord(&pos, &pos, &mWorld);
        m_lit.Position = pos;
		
        pDevice->SetLight(m_index, &m_lit);
        pDevice->LightEnable(m_index, TRUE);
        return true;
    }

    void draw(IDirect3DDevice9* pDevice)
    {
        if (NULL == pDevice)
            return;
        D3DXMATRIX m;
        D3DXMatrixTranslation(&m, m_lit.Position.x, m_lit.Position.y, m_lit.Position.z);
        pDevice->SetTransform(D3DTS_WORLD, &m);
        pDevice->SetMaterial(&d3d::WHITE_MTRL);
        m_pMesh->DrawSubset(0);
    }

    D3DXVECTOR3 getPosition(void) const { return D3DXVECTOR3(m_lit.Position); }

private:
    DWORD               m_index;
    D3DXMATRIX          m_mLocal;
    D3DLIGHT9           m_lit;
    ID3DXMesh*          m_pMesh;
    d3d::BoundingSphere m_bound;
};


// -----------------------------------------------------------------------------
// Global variables
// -----------------------------------------------------------------------------
CWall	g_legoPlane;
CWall	g_legowall[3];
CSphere	g_sphere[totalBalls];
CSphere	g_target_greyball;
CSphere g_target_redball;
CLight	g_light;
CWall g_legoLine;

LPD3DXFONT g_Lifecount, g_gameover, g_LifeLabel, g_StartLabel, g_gameclear;

double g_camera_pos[3] = {0.0, 5.0, -8.0};

// -----------------------------------------------------------------------------
// Functions
// -----------------------------------------------------------------------------


void destroyAllLegoBlock(void)
{
	for (int i = 0; i < totalBalls; i++) {
		g_sphere[i].destroy();
	}
}

// initialization
bool Setup()
{
	int i;
	
    D3DXMatrixIdentity(&g_mWorld);
    D3DXMatrixIdentity(&g_mView);
    D3DXMatrixIdentity(&g_mProj);
		
	// create plane and set the position
    if (false == g_legoPlane.create(Device, -1, -1, 6, 0.03f, 9, d3d::GREEN)) return false;
    g_legoPlane.setPosition(0.0f, -0.0006f / 5, 0.0f);

	// create line and set the postion
	if (false == g_legoLine.create(Device, -1, -1, 6, 0.1f, 0.1f, d3d::BLUE)) return false;
	g_legoLine.setPosition(0.0f, -0.0006f / 5, initialGreyBallPosZ);
	
	// create walls and set the position. note that there are four walls
	if (false == g_legowall[0].create(Device, -1, -1, horizontalBarWidth, 0.3f, wallThickness, d3d::DARKRED)) return false;
	g_legowall[0].setPosition(0.0f, wallThickness, verticalBarDepth/2);
	if (false == g_legowall[1].create(Device, -1, -1, wallThickness, 0.3f, verticalBarDepth, d3d::DARKRED)) return false;
	g_legowall[1].setPosition(horizontalBarWidth/2, wallThickness, 0.0f);
	if (false == g_legowall[2].create(Device, -1, -1, wallThickness, 0.3f, verticalBarDepth, d3d::DARKRED)) return false;
	g_legowall[2].setPosition(-horizontalBarWidth/2, wallThickness, 0.0f);

	// create all balls and set the position
	for (i=0;i<totalBalls;i++) {
		if (false == g_sphere[i].create(Device, ballColor)) return false;
		g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS , spherePos[i][1]);
		g_sphere[i].setPower(0,0);
	}
	
	// create red ball for set direction
	if (false == g_target_redball.create(Device, d3d::RED)) return false;
	g_target_redball.setCenter(.0f, (float)M_RADIUS, initialRedBallPosZ);

	// create grey ball for set direction
    if (false == g_target_greyball.create(Device, d3d::GREY)) return false;
	g_target_greyball.setCenter(.0f, (float)M_RADIUS, initialGreyBallPosZ);
	
	// light setting 
    D3DLIGHT9 lit;
    ::ZeroMemory(&lit, sizeof(lit));
    lit.Type         = D3DLIGHT_POINT;
    lit.Diffuse      = d3d::WHITE; 
	lit.Specular     = d3d::WHITE * 0.9f;
    lit.Ambient      = d3d::WHITE * 0.9f;
    lit.Position     = D3DXVECTOR3(0.0f, 3.0f, 0.0f);
    lit.Range        = 100.0f;
    lit.Attenuation0 = 0.0f;
    lit.Attenuation1 = 0.9f;
    lit.Attenuation2 = 0.0f;
    if (false == g_light.create(Device, lit, (float)0.0))
        return false;
	
	// Position and aim the camera.
	D3DXVECTOR3 pos(0.0f, 10.0f, -9.0f);
	D3DXVECTOR3 target(0.0f, 0.0f, 0.0f);
	D3DXVECTOR3 up(0.0f, 2.0f, 0.0f);
	D3DXMatrixLookAtLH(&g_mView, &pos, &target, &up);
	Device->SetTransform(D3DTS_VIEW, &g_mView);
	
	// Set the projection matrix.
	D3DXMatrixPerspectiveFovLH(&g_mProj, D3DX_PI / 4,
        (float)Width / (float)Height, 1.0f, 100.0f);
	Device->SetTransform(D3DTS_PROJECTION, &g_mProj);
	
    // Set render states.
    Device->SetRenderState(D3DRS_LIGHTING, TRUE);
    Device->SetRenderState(D3DRS_SPECULARENABLE, TRUE);
    Device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	
	// render texts
	D3DXCreateFont(Device, 50, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &g_Lifecount);
	D3DXCreateFont(Device, 40, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &g_gameover);
	D3DXCreateFont(Device, 60, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &g_LifeLabel);
	D3DXCreateFont(Device, 30, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &g_StartLabel);
	D3DXCreateFont(Device, 50, 0, FW_BOLD, 0, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, TEXT("Arial"), &g_gameclear);
	// set light
	g_light.setLight(Device, g_mWorld);
	return true;
}

// reset all position
void resetAllPositions(void) {
	// reset positions
	for (int i = 0; i < totalBalls; i++) {
		g_sphere[i].setCenter(spherePos[i][0], (float)M_RADIUS, spherePos[i][1]);
		g_sphere[i].setPower(0, 0);
	}

	// reset red ball
	g_target_redball.setCenter(.0f, (float)M_RADIUS, initialRedBallPosZ);
	g_target_redball.setPower(0.0, 0.0);

	// reset grey ball
	g_target_greyball.setCenter(.0f, (float)M_RADIUS, initialGreyBallPosZ);
	g_target_greyball.setPower(0.0, 0.0);
}

void resetRedAndGreyBalls(void) {
	g_target_redball.setCenter(.0f, (float)M_RADIUS, initialRedBallPosZ);
	g_target_redball.setPower(0.0, 0.0);

	g_target_greyball.setCenter(.0f, (float)M_RADIUS, initialGreyBallPosZ);
	g_target_greyball.setPower(0.0, 0.0);
}

void renderTexts(void) {
	// render texts
	//Set text
	D3DCOLOR fontColor = D3DCOLOR_ARGB(255, 166, 234, 93);
	D3DCOLOR fontColorstart = D3DCOLOR_ARGB(255, 0, 0, 0);
	RECT LifeLabelRect;
	RECT LifecountLabelRect;
	RECT ScoreLabelRect;
	RECT gameoverRect;
	RECT gamestart;
	RECT gameclear;

	LifeLabelRect.left = 20;
	LifeLabelRect.right = 150;
	LifeLabelRect.top = 20;
	LifeLabelRect.bottom = 100;

	LifecountLabelRect.left = 55;
	LifecountLabelRect.right = 200;
	LifecountLabelRect.top = 70;
	LifecountLabelRect.bottom = 180;

	gameoverRect.left = 410;
	gameoverRect.right = 650;
	gameoverRect.top = 500;
	gameoverRect.bottom = 580;

	gameclear.left = 440;
	gameclear.right = 650;
	gameclear.top = 500;
	gameclear.bottom = 580;

	gamestart.left = 380;
	gamestart.right = 680;
	gamestart.top = 400;
	gamestart.bottom = 480;

	// Draw some text
	char LifeLabelBuffer[20] = "Lives Left";
	char ScoreLabelBuffer[20] = "Score";
	char gameoverBuffer[10] = "Game over";
	char gameclearBuffer[10] = "CLEAR";
	char gamestartBuffer[30] = "Press SPACE to start";

	switch (life)
	{
	case 5:
		g_Lifecount->DrawText(NULL, "5", -1, &LifecountLabelRect, 0, fontColor);
		break;
	case 4:
		g_Lifecount->DrawText(NULL, "4", -1, &LifecountLabelRect, 0, fontColor);
		break;
	case 3:
		g_Lifecount->DrawText(NULL, "3", -1, &LifecountLabelRect, 0, fontColor);
		break;
	case 2:
		g_Lifecount->DrawText(NULL, "2", -1, &LifecountLabelRect, 0, fontColor);
		break;
	case 1:
		g_Lifecount->DrawText(NULL, "1", -1, &LifecountLabelRect, 0, fontColor);
		break;
	case 0:
		g_Lifecount->DrawText(NULL, "0", -1, &LifecountLabelRect, 0, fontColor);
		break;
	}

	g_LifeLabel->DrawText(NULL, LifeLabelBuffer, -1, &LifeLabelRect, 0, fontColor);


	if (!isRoundStarted && life > 0 && score < (int)MAXSCORE) {
		// when a round is ended but still has lives and score is not MAX
		// draw start text
		g_StartLabel->DrawText(NULL, gamestartBuffer, -1, &gamestart, 0, fontColorstart);
	}
	if (life <= 0) {
		// if no lives left, all rounds are ended
		fontColor = D3DCOLOR_ARGB(255, 255, 0, 0);
		g_gameover->DrawTextA(NULL, gameoverBuffer, -1, &gameoverRect, 0, fontColor);
		fontColor = D3DCOLOR_ARGB(255, 0, 0, 255);
		isGameEnded = true;
	}
	if (score >= (int)MAXSCORE && life > 0) {
		// when score is MAX and lives left, draw game clear message
		g_gameclear->DrawTextA(NULL, gameclearBuffer, -1, &gameclear, 0, fontColor);
		isGameEnded = true;
	}
}

void Cleanup(void)
{
    g_legoPlane.destroy();
	for(int i = 0 ; i < 3; i++) {
		g_legowall[i].destroy();
	}
    destroyAllLegoBlock();
    g_light.destroy();
}


// timeDelta represents the time between the current image frame and the last image frame.
// the distance of moving balls should be "velocity * timeDelta"
bool Display(float timeDelta)
{
	int i=0;
	int j = 0;


	if (Device)
	{
		Device->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00afafaf, 1.0f, 0);
		Device->BeginScene();

		if (isGameEnded) {
			resetAllPositions();
		}
		else {
			// update the red ball
			g_target_redball.ballUpdate(timeDelta);
			D3DXVECTOR3 redballCenter = g_target_redball.getCenter();
			if (redballCenter.z <= -4.0f + M_RADIUS) {
				g_target_redball.setPower(0.0, 0.0);
				life--;
				isRoundStarted = false;

				// reset positions
				if (life < 1) {
					resetAllPositions();
				}
				else {
					resetRedAndGreyBalls();
				}
			}
			else {
				// update the position of each ball. during update, check whether each ball hit by walls.
				for (i = 0; i < 3; i++) {
					for (j = 0; j < totalBalls; j++) {
						g_legowall[i].hitBy(g_sphere[j]);
						g_legowall[i].hitBy(g_target_redball);
						if (life < 0) {
							g_target_redball.setCenter(.0f, (float)M_RADIUS, initialRedBallPosZ);
							g_target_redball.setPower(0.0, 0.0);
							g_target_greyball.setCenter(.0f, (float)M_RADIUS, initialGreyBallPosZ);
						}
					}
				}

				// check whether any two balls hit together and update the direction of balls
				for (i = 0; i < totalBalls; i++) {
					g_sphere[i].hitBy(g_target_redball);
				}
			}
		}
		
		// draw plane, walls, and spheres
		g_legoPlane.draw(Device, g_mWorld);
		for (i=0;i<3;i++) 	{
			g_legowall[i].draw(Device, g_mWorld);
		}
		for (i = 0; i < totalBalls; i++) {
			g_sphere[i].draw(Device, g_mWorld);
		}

		// check if grey ball and red ball had collision
		if (isRoundStarted) {
			g_target_greyball.hitBy(g_target_redball);
		}

		g_target_redball.draw(Device, g_mWorld);
		g_target_greyball.draw(Device, g_mWorld);
		g_legoLine.draw(Device, g_mWorld);
        g_light.draw(Device);
		
		renderTexts();

		Device->EndScene();
		Device->Present(0, 0, 0, 0);
		Device->SetTexture( 0, NULL );
	}
	return true;
}

LRESULT CALLBACK d3d::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static bool wire = false;
	static bool isReset = true;
    static int old_x = 0;
    static int old_y = 0;
    static enum { WORLD_MOVE, LIGHT_MOVE, BLOCK_MOVE } move = WORLD_MOVE;
	
	D3DXVECTOR3 ballCenter;
	switch (msg) {
	case WM_DESTROY:
	{
		::PostQuitMessage(0);
		break;
	}
	case WM_KEYDOWN:
	{
		switch (wParam) {
		case VK_ESCAPE:
			::DestroyWindow(hwnd);
			break;
		case VK_RETURN:
			if (NULL != Device) {
				wire = !wire;
				Device->SetRenderState(D3DRS_FILLMODE,
					(wire ? D3DFILL_WIREFRAME : D3DFILL_SOLID));
			}
			break;
		case VK_SPACE:

			// start game on space key down
			if (life > 0 && !isRoundStarted) {
				g_target_redball.setPower(REDBALLSPEED, REDBALLSPEED);
				isRoundStarted = true;
			}

			break;
		case VK_LEFT:
			ballCenter = g_target_greyball.getCenter();
			if ((ballCenter.x - KEYSTEP) > (g_legowall[2].getPositionX() + g_legowall[2].getWidth() / 2)) {
				g_target_greyball.setCenter(ballCenter.x - KEYSTEP, ballCenter.y, ballCenter.z);
				g_target_greyball.setVelocity_X(-KEYSTEP * 5);
			}
			break;
		case VK_RIGHT:
			ballCenter = g_target_greyball.getCenter();
			if ((ballCenter.x + KEYSTEP) < (g_legowall[1].getPositionX() - g_legowall[1].getWidth() / 2)) {
				g_target_greyball.setCenter(ballCenter.x + KEYSTEP, ballCenter.y, ballCenter.z);
				g_target_greyball.setVelocity_X(KEYSTEP * 5);
			}
			break;
		}
	}
	}
	return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hinstance,
				   HINSTANCE prevInstance, 
				   PSTR cmdLine,
				   int showCmd)
{
    srand(static_cast<unsigned int>(time(NULL)));
	
	if(!d3d::InitD3D(hinstance,
		Width, Height, true, D3DDEVTYPE_HAL, &Device))
	{
		::MessageBox(0, "InitD3D() - FAILED", 0, 0);
		return 0;
	}
	
	if(!Setup())
	{
		::MessageBox(0, "Setup() - FAILED", 0, 0);
		return 0;
	}
	

	d3d::EnterMsgLoop( Display );
	
	Cleanup();
	
	Device->Release();
	
	return 0;
}