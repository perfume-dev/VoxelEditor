#pragma once

const int NUM_CELL = 80;
const unsigned int CELL_SIZE = 5;
const float EDITOR_SIZE_IN_CM = 200;

const unsigned int VOXEL_TAG = 100;
const unsigned int FLOOR_TAG = 101;

const unsigned int HANDLE_TAG = 200;
const unsigned int HANDLE_X_TAG = 201;
const unsigned int HANDLE_Y_TAG = 202;
const unsigned int HANDLE_Z_TAG = 203;
const unsigned int HANDLE_NEG_X_TAG = 204;
const unsigned int HANDLE_NEG_Y_TAG = 205;
const unsigned int HANDLE_NEG_Z_TAG = 206;

inline ofMatrix4x4 getGridToWorldMatrix() {
	ofMatrix4x4 gridToWorldMatrix;
	
	float n = NUM_CELL - 1;
	
	float s = EDITOR_SIZE_IN_CM / (float)n;
	gridToWorldMatrix.glScale(s, s, s);
	gridToWorldMatrix.glTranslate(-n / 2, 0, -n / 2);
	
	return gridToWorldMatrix;
}

inline void billboard()
{
    ofMatrix4x4 m;
    glGetFloatv(GL_MODELVIEW_MATRIX, m.getPtr());
    
    ofVec3f s = m.getScale();
    
    m(0, 0) = s.x;
    m(0, 1) = 0;
    m(0, 2) = 0;
    
    m(1, 0) = 0;
    m(1, 1) = s.y;
    m(1, 2) = 0;
    
    m(2, 0) = 0;
    m(2, 1) = 0;
    m(2, 2) = s.z;
    
    glLoadMatrixf(m.getPtr());
}
