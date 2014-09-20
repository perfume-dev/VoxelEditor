#pragma once

#include "ControlOF.h"

#include "Constance.h"
#include "VoxelData.h"

class Editor
{
public:
	
	enum EditMode
	{
		EDITMODE_PUT,
		EDITMODE_MOVE,
		EDITMODE_RESIZE,
		EDITMODE_PICK_COLOR
	} editmode;
	
	void setup()
	{
		gridToWorldMatrix = getGridToWorldMatrix();
		worldToGridMatrix.makeInvertOf(gridToWorldMatrix);

		orbit.set(0 , -15, 150);
		orbit_t = orbit;
		
		cursor.set(40, 30, 30);
		cursor_t = cursor;

		voxel_color.set(255);
		selected_voxel = NULL;
		focused_voxel = NULL;
		
		cam.setFov(60);

		setupMesh();
		setupUI();
		
		editmode = EDITMODE_PUT;
		put_failed = false;
		
		json_filename = "default.json";
	}

	void update()
	{
		cursor_t += (cursor - cursor_t) * 0.5;
		updateCamera();
	}

	void draw()
	{
		glGetIntegerv(GL_VIEWPORT, viewport);

		glPushAttrib(GL_ALL_ATTRIB_BITS);

		ofEnableAlphaBlending();

		light1.enable();

		light1.setDiffuseColor(ofFloatColor(0.2, 0.2, 0.2));
		light1.setAmbientColor(ofFloatColor(0.1, 0.1, 0.1));
		light1.setSpecularColor(ofFloatColor(0.3, 0.3, 0.3));

		light1.setPosition(0, -200, 500);

		ofSetGlobalAmbientColor(ofColor(190, 190, 190));

		cam.begin();

		light2.enable();
		light2.setDiffuseColor(ofFloatColor(0.3, 0.3, 0.3));
		light2.setPosition(0, 400, 300);

		glPushMatrix();
		{
			{
				glDisable(GL_DEPTH_TEST);

				ofVec3f off = gridToWorldMatrix.preMult(cursor_t);
				glTranslatef(-off.x, -off.y, -off.z);

				glPushMatrix();
				{
					glMultMatrixf(gridToWorldMatrix.getPtr());

					glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
					glGetDoublev(GL_PROJECTION_MATRIX, projection);

					drawFloor();

					ofEnableLighting();
					glEnable(GL_DEPTH_TEST);

					drawVoxel();
				}
				glPopMatrix();

				ofDisableLighting();

				glPushMatrix();
				{
					glMultMatrixf(gridToWorldMatrix.getPtr());
					glDisable(GL_DEPTH_TEST);
					drawCursor();
					drawVoxelMarker();
				}
				glPopMatrix();
			}
		}
		glPopMatrix();

		light2.disable();
		cam.end();

		light1.disable();

		ofDisableLighting();
		glPopAttrib();
	}

public:
	void moveCamera(float x, float y, float z = 0)
	{
		orbit_t += ofVec3f(x, y, z);
		orbit_t.y = ofClamp(orbit_t.y, -90, 90);
		orbit_t.z = ofClamp(orbit_t.z, 50, 600);
	}

	void offsetCamera(float x, float y)
	{
		offset_t += cam.getOrientationQuat() * ofVec3f(x, y);
	}

	void moveCursor(float x, float y, float z = 0)
	{
		ofVec3f v = cam.getGlobalOrientation() * (ofVec3f(x, y, z));

		float d = sqrt(2) / 2;

		ofVec3f m;

		if (v.x > d)
			m = ofVec3f(1, 0, 0);
		else if (v.x < -d)
			m = ofVec3f(-1, 0, 0);

		if (v.y > d)
			m = ofVec3f(0, 1, 0);
		else if (v.y < -d)
			m = ofVec3f(0, -1, 0);

		if (v.z > d)
			m = ofVec3f(0, 0, 1);
		else if (v.z < -d)
			m = ofVec3f(0, 0, -1);

		cursor += m;

		if (cursor.x <= 0) cursor.x = 0;
		if (cursor.x >= 80) cursor.x = 80 - 1;

		if (cursor.y <= 0) cursor.y = 0;
		if (cursor.y >= 2000) cursor.y = 2000 - 1;

		if (cursor.z <= 0) cursor.z = 0;
		if (cursor.z >= 60) cursor.z = 60 - 1;
	}

	void onMoved(int x, int y)
	{
		focused_voxel = voxel_hittest(x, y);
	}

	void onPressed(int x, int y)
	{
		half_selected_voxel = voxel_hittest(x, y);
		
		if (editmode == EDITMODE_RESIZE
			|| editmode == EDITMODE_MOVE)
		{
			unsigned int handle = handle_hittest(x, y);
			
			if (handle != 0 && selected_voxel != NULL)
			{
				int amt = 1;
				if (ofGetModifierPressed(OF_KEY_SHIFT)) amt *= -1;
				
				pushUndoBuffer();
				
				if (editmode == EDITMODE_RESIZE)
				{
					switch (handle)
					{
						case HANDLE_X_TAG:
							selected_voxel->w += amt;
							cursor.x += amt;
							break;
						case HANDLE_Y_TAG:
							selected_voxel->h += amt;
							cursor.y += amt;
							break;
						case HANDLE_Z_TAG:
							selected_voxel->d += amt;
							cursor.z += amt;
							break;
						case HANDLE_NEG_X_TAG:
							selected_voxel->w += amt;
							selected_voxel->x -= amt;
							cursor.x -= amt;
							break;
						case HANDLE_NEG_Y_TAG:
							selected_voxel->h += amt;
							selected_voxel->y -= amt;
							cursor.y -= amt;
							break;
						case HANDLE_NEG_Z_TAG:
							selected_voxel->d += amt;
							selected_voxel->z -= amt;
							cursor.z -= amt;
							break;
					}
				} else if (editmode == EDITMODE_MOVE)
				{
					switch (handle)
					{
						case HANDLE_X_TAG:
							selected_voxel->x += amt;
							cursor.x += amt;
							break;
						case HANDLE_Y_TAG:
							selected_voxel->y += amt;
							cursor.y += amt;
							break;
						case HANDLE_Z_TAG:
							selected_voxel->z += amt;
							cursor.z += amt;
							break;
						case HANDLE_NEG_X_TAG:
							selected_voxel->x -= amt;
							cursor.x -= amt;
							break;
						case HANDLE_NEG_Y_TAG:
							selected_voxel->y -= amt;
							cursor.y -= amt;
							break;
						case HANDLE_NEG_Z_TAG:
							selected_voxel->z -= amt;
							cursor.z -= amt;
							break;
					}
				}
			}
		}
		
		if (editmode == EDITMODE_PICK_COLOR)
		{
			if (half_selected_voxel)
			{
				setColor(half_selected_voxel->color);
			}
		}
	}

	void onDoubleClick(int x, int y)
	{
		if (editmode == EDITMODE_PUT)
		{
			if (put_voxel_hittest(x, y))
				put();
		}
		else if (selected_voxel)
		{
			cursor = selected_voxel->center();
		}
	}
	
	void onReleased(int x, int y)
	{
		VoxelData* o = voxel_hittest(x, y);
		if (o != NULL
			&& half_selected_voxel == o)
		{
			selected_voxel = o;
		}
		
		half_selected_voxel = NULL;
	}
	
	void onResized(int w, int h)
	{
		right_group->setPosition(w - (180 + 4), 4, 0);
	}

	const ofVec3f& getCursorPos() { return cursor; }
	
	void put()
	{
		if (editmode != EDITMODE_PUT) return;
		
		{
			// check voxels
			const vector<VoxelData>& arr = voxels.getVoxels();
			for (int i = 0; i < arr.size(); i++)
			{
				const VoxelData& v = arr[i];
				if (v.isInside(cursor.x, cursor.y, cursor.z))
				{
					// can replace only W/H/D == 1 voxel
					if (v.w > 1 || v.h > 1 || v.d > 1)
					{
						put_failed = true;
						return false;
					}
				}
			}
		}
		
		pushUndoBuffer();
		
		VoxelData v;
		
		v.x = cursor.x;
		v.y = cursor.y;
		v.z = cursor.z;
		
		v.w = 1;
		v.h = 1;
		v.d = 1;
		
		v.color = voxel_color;
		
		voxels.add(v);
		
		selected_voxel = &voxels.getVoxels().back();
		focused_voxel = &voxels.getVoxels().back();
	}
	
	void remove()
	{
		if (selected_voxel != NULL)
		{
			pushUndoBuffer();
			voxels.remove(*selected_voxel);
			selected_voxel = NULL;
		}
		else if (voxels.exists(cursor))
		{
			pushUndoBuffer();
			voxels.remove(cursor);
		}
	}

	void setEditMode(EditMode m)
	{
		for (int i = 0; i < tool_group.size(); i++)
		{
			tool_group[i]->setValue(false);
		}
		tool_group[m]->setValue(true);
		editmode = m;
	}

private:
	Voxel voxels;

	ofMatrix4x4 gridToWorldMatrix;
	ofMatrix4x4 worldToGridMatrix;

	ofCamera cam;

	ofLight light1, light2;

	ofColor voxel_color;
	ofVec3f cursor, cursor_t;

	VoxelData* half_selected_voxel;
	VoxelData* selected_voxel;
	VoxelData* focused_voxel;

	ofVec3f orbit, orbit_t;
	ofVec3f offset, offset_t;
	
	bool put_failed;

private:
	void updateCamera()
	{
		orbit += (orbit_t - orbit) * 0.2;
		offset += (offset_t - offset) * 0.2;

		ofMatrix4x4 m;
		m.glTranslate(offset);
		m.glRotate(orbit.x, 0, 1, 0);
		m.glRotate(orbit.y, 1, 0, 0);
		m.glTranslate(0, 0, orbit.z);
		m.glTranslate(-offset);

		m.preMultTranslate(offset);

		cam.setTransformMatrix(m);
	}

	void drawFloor()
	{
		const float n = NUM_CELL;

		glPushMatrix();
		glTranslatef(-0.5, -0.5, -0.5);

		ofFill();

		glPushMatrix();
		glRotatef(90, 1, 0, 0);

		bool c = false;
		for (int x = 0; x < 80; x++)
		{
			c = (x % 2) == 0;
			for (int y = 0; y < 60; y++)
			{
				if (c)
					ofSetColor(90);
				else
					ofSetColor(80);
				c = !c;
				ofRect(x, y, 1, 1);
			}
		}

		glPopMatrix();

		ofNoFill();
		ofPushMatrix();
		ofScale(80, 60, 60);
		ofDrawBox(0.5, 0.5, 0.5, 1);
		ofPopMatrix();

		glPopMatrix();
	}

	void drawVoxel()
	{
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		glPushMatrix();
		ofNoFill();

		{
			glEnable(GL_DEPTH_TEST);

			const vector<VoxelData>& arr = voxels.getVoxels();

			for (int i = 0; i < arr.size(); i++)
			{
				const VoxelData& v = arr[i];

				glPushName(i);
				drawVoxelData(v, true);
				glPopName();
			}
		}

		glPopMatrix();
		glPopAttrib();
	}

	void drawVoxelMarker()
	{
		if (editmode == EDITMODE_PUT) return;
		
		if (focused_voxel)
		{
			ofSetColor(255, 64);
			drawVoxelData(*focused_voxel, false, false);
		}

		if (selected_voxel)
		{
			ofSetColor(255);
			drawVoxelData(*selected_voxel, false, false);
		}

		drawHandle();
	}

	void drawHandleArrow(bool inv, const ofQuaternion& q = ofQuaternion())
	{
		ofPushMatrix();
		ofMultMatrix(q);
		if (inv) glScalef(-1, 1, 1);
		ofLine(0, 0, 0.8, 0);
		ofLine(0.8, 0.0, 0.6, 0.2);
		ofLine(0.8, 0.0, 0.6, -0.2);
		ofPopMatrix();
	}

	void drawHandle()
	{
		if ((editmode == EDITMODE_MOVE
			 || editmode == EDITMODE_RESIZE) == false) return;

		if (selected_voxel)
		{
			VoxelData* v = selected_voxel;

			ofFill();

			float handle_size = 0.2;
			bool inv = ofGetModifierPressed(OF_KEY_SHIFT);

			glPushName(HANDLE_TAG);

			// X
			ofPushMatrix();
			ofSetColor(255, 0, 0);
			ofTranslate(v->x + v->w * 1.0, v->y + v->h * 0.5,
						v->z + v->d * 0.5);
			drawHandleArrow(inv);

			billboard();
			glPushName(HANDLE_X_TAG);
			ofCircle(0, 0, handle_size);
			glPopName();
			ofPopMatrix();

			// Y
			ofPushMatrix();
			ofSetColor(0, 255, 0);
			ofTranslate(v->x + v->w * 0.5, v->y + v->h * 1.0,
						v->z + v->d * 0.5);
			drawHandleArrow(inv, ofQuaternion(90, ofVec3f(0, 0, 1)));

			billboard();
			glPushName(HANDLE_Y_TAG);
			ofCircle(0, 0, handle_size);
			glPopName();
			ofPopMatrix();

			// Z
			ofPushMatrix();
			ofSetColor(0, 0, 255);
			ofTranslate(v->x + v->w * 0.5, v->y + v->h * 0.5,
						v->z + v->d * 1.0);
			drawHandleArrow(inv, ofQuaternion(90, ofVec3f(0, -1, 0)));

			billboard();
			glPushName(HANDLE_Z_TAG);
			ofCircle(0, 0, handle_size);
			glPopName();
			ofPopMatrix();

			// -X
			ofPushMatrix();
			ofSetColor(255, 0, 0);
			ofTranslate(v->x + v->w * 0.0, v->y + v->h * 0.5,
						v->z + v->d * 0.5);
			drawHandleArrow(inv, ofQuaternion(180, ofVec3f(0, 0, 1)));

			billboard();
			glPushName(HANDLE_NEG_X_TAG);
			ofCircle(0, 0, handle_size);
			glPopName();
			ofPopMatrix();

			// -Y
			ofPushMatrix();
			ofSetColor(0, 255, 0);
			ofTranslate(v->x + v->w * 0.5, v->y + v->h * 0.0,
						v->z + v->d * 0.5);
			drawHandleArrow(inv, ofQuaternion(90, ofVec3f(0, 0, -1)));

			billboard();
			glPushName(HANDLE_NEG_Y_TAG);
			ofCircle(0, 0, handle_size);
			glPopName();
			ofPopMatrix();

			// -Z
			ofPushMatrix();
			ofSetColor(0, 0, 255);
			ofTranslate(v->x + v->w * 0.5, v->y + v->h * 0.5,
						v->z + v->d * 0.0);
			drawHandleArrow(inv, ofQuaternion(90, ofVec3f(0, 1, 0)));

			billboard();
			glPushName(HANDLE_NEG_Z_TAG);
			ofCircle(0, 0, handle_size);
			glPopName();
			ofPopMatrix();

			glPopName();
		}
	}

	void drawCursor()
	{
		if (editmode != EDITMODE_PUT) return;
		
		if (put_failed)
			ofSetColor(255, 0, 0);
		else
			ofSetColor(255);
		
		put_failed = false;
		drawBoxOutline(cursor.x, cursor.y, cursor.z);
	}

private:  // hittest
	GLdouble modelview[16], projection[16];
	GLint viewport[4];

	struct Selection
	{
		GLfloat min_depth, max_depth;
		vector<GLuint> name_stack;

		static bool sort_by_depth(const Selection& a, const Selection& b)
		{
			return a.min_depth < b.min_depth;
		}
	};

	vector<Selection> pickup(int x, int y);
	vector<GLuint> hittest(int x, int y, unsigned int tag_name);

	VoxelData* voxel_hittest(int x, int y)
	{
		// TODO: cache name_stack on frame
		vector<GLuint> name_stack = hittest(x, y, VOXEL_TAG);
		if (name_stack.size() != 2) return NULL;

		unsigned int oid = name_stack[1];
		VoxelData& voxel = voxels.getVoxels().at(oid);
		return &voxel;
	}

	unsigned int handle_hittest(int x, int y)
	{
		vector<GLuint> name_stack = hittest(x, y, HANDLE_TAG);
		if (name_stack.size() != 2) return 0;
		return name_stack[1];
	}
	
	bool put_voxel_hittest(int x, int y)
	{
		vector<Selection> picked_stack = pickup(x, y);
		if (picked_stack.empty()) return false;
		
		Selection &sel = picked_stack[0];
		
		if (sel.name_stack[0] != VOXEL_TAG) return false;
		
		unsigned int oid = sel.name_stack[1];
		VoxelData& v = voxels.getVoxels().at(oid);
		
		ofVec3f p(v.x + 0.5, v.y + 0.5, v.z + 0.5);
		
		GLdouble ox = 0, oy = 0, oz = 0;
		GLdouble vx = 0, vy = 0, vz = 0;
		
		gluProject(p.x, p.y, p.z,
				   modelview, projection, viewport,
				   &vx, &vy, &vz);
		
		vy = ofGetHeight() - vy;
		
		{
			gluUnProject(x, ofGetHeight() - y, sel.min_depth,
						 modelview, projection, viewport,
						 &ox, &oy, &oz);
			
			ofVec3f axis = (p - ofVec3f(ox, oy, oz));
			ofVec3f norm(fabs(axis.x), fabs(axis.y), fabs(axis.z));
			
			if (norm.x > max(norm.y, norm.z))
			{
				if (axis.x > 0)
					p.x -= 1;
				else
					p.x += 1;
			}
			else if (norm.y > max(norm.x, norm.z))
			{
				if (axis.y > 0)
					p.y -= 1;
				else
					p.y += 1;
			}
			else if(norm.z > max(norm.x, norm.y))
			{
				if (axis.z > 0)
					p.z -= 1;
				else
					p.z += 1;
			}
			
			cursor.x = floor(p.x);
			cursor.y = floor(p.y);
			cursor.z = floor(p.z);
		}
		
		return true;
	}

private:
	ofVbo box_mesh;
	ofVbo box_wireframe_mesh;

	void setupMesh()
	{
		// box mesh
		{
			float size = 1;

			{
				ofBoxPrimitive box(size, size, size, 1, 1, 1);

				ofMesh& m = box.getMesh();
				vector<ofVec3f>& v = m.getVertices();
				ofVec3f off(size / 2, size / 2, size / 2);

				for (int i = 0; i < v.size(); i++)
				{
					v[i] += off;
				}

				box_mesh.setMesh(box.getMesh(), GL_STATIC_DRAW);
			}

			{
				ofMesh mesh;
				mesh.addVertex(ofVec3f(0, 0, 0));
				mesh.addVertex(ofVec3f(size, 0, 0));
				mesh.addVertex(ofVec3f(0, size, 0));
				mesh.addVertex(ofVec3f(size, size, 0));

				mesh.addVertex(ofVec3f(0, 0, size));
				mesh.addVertex(ofVec3f(size, 0, size));
				mesh.addVertex(ofVec3f(0, size, size));
				mesh.addVertex(ofVec3f(size, size, size));

				mesh.addIndex(0);
				mesh.addIndex(1);

				mesh.addIndex(0);
				mesh.addIndex(2);

				mesh.addIndex(1);
				mesh.addIndex(3);

				mesh.addIndex(2);
				mesh.addIndex(3);

				for (int i = 0; i < 4; i++)
				{
					mesh.addIndex(i);
					mesh.addIndex(i + 4);
				}

				mesh.addIndex(0 + 4);
				mesh.addIndex(1 + 4);

				mesh.addIndex(0 + 4);
				mesh.addIndex(2 + 4);

				mesh.addIndex(1 + 4);
				mesh.addIndex(3 + 4);

				mesh.addIndex(2 + 4);
				mesh.addIndex(3 + 4);

				box_wireframe_mesh.setMesh(mesh, GL_STATIC_DRAW);
			}
		}
	}

	void drawVoxelData(const VoxelData& voxel, bool fill = true,
					   bool with_color = true)
	{
		const VoxelData& v = voxel;

		if (with_color) glColor3ub(v.color.r, v.color.g, v.color.b);
		if (fill)
			drawBox(v.x, v.y, v.z, v.w, v.h, v.d);
		else
			drawBoxOutline(v.x, v.y, v.z, v.w, v.h, v.d);

	}
	
	void drawBox(int x, int y, int z, int w = 1, int h = 1, int d = 1)
	{
		glPushMatrix();
		glTranslatef(x, y, z);
		glScalef(w, h, d);
		box_mesh.drawElements(GL_TRIANGLES, 36);
		glPopMatrix();
	}
	
	void drawBoxOutline(int x, int y, int z, int w = 1, int h = 1, int d = 1)
	{
		glPushMatrix();
		glTranslatef(x, y, z);
		glScalef(w, h, d);
		box_wireframe_mesh.drawElements(GL_LINES, 24);
		glPopMatrix();
	}
	
private: // UI
	
	ofxControlGroup *left_group;
	ofxControlGroup *right_group;

	ofxControl c;
	ofxControlSliderI* color_sliders[3];
	ofxControlColorPicker *picker;
	
	vector<ofxControlButton*> tool_group;
	
	void setupUI()
	{
		c.setup();
		
		// left
		
		left_group = c.begin(4, 4);
		
		{
			ofxControlButton *o;
			
			o = c.addButton("save json");
			ofAddListener(o->pressed, this, &Editor::onSavePressed);
			
			o = c.addButton("load json");
			ofAddListener(o->pressed, this, &Editor::onLoadPressed);
			
            o = c.addButton("load *.obj");
            ofAddListener(o->pressed, this, &Editor::onLoadObjPressed);
            
			c.addSeparator();
			
			o = c.addButton("clear");
			ofAddListener(o->pressed, this, &Editor::onClear);
			
			o = c.addButton("undo");
			ofAddListener(o->pressed, this, &Editor::onUndo);
			
			c.addSeparator();
			
			tool_group.clear();
			
			o = c.addButton("put");
			o->setValue(true);
			o->setToggle(true);
			tool_group.push_back(o);
			ofAddListener(o->pressed, this, &Editor::onChangeTool);

			o = c.addButton("move");
			o->setToggle(true);
			tool_group.push_back(o);
			ofAddListener(o->pressed, this, &Editor::onChangeTool);

			o = c.addButton("resize");
			o->setToggle(true);
			tool_group.push_back(o);
			ofAddListener(o->pressed, this, &Editor::onChangeTool);
			
			o = c.addButton("pick color");
			o->setToggle(true);
			tool_group.push_back(o);
			ofAddListener(o->pressed, this, &Editor::onChangeTool);
		}
		
		c.end();
		
		// right
		
		right_group = c.begin(ofGetWidth() - (180 + 4), 4);
		
		{
			vector<unsigned int> colors;
			
			colors.push_back(0x000000);
			colors.push_back(0x222222);
			colors.push_back(0x444444);
			colors.push_back(0x777777);
			colors.push_back(0x999999);
			colors.push_back(0xbbbbbb);
			colors.push_back(0xdddddd);
			colors.push_back(0xffffff);
			colors.push_back(0x542f23);
			colors.push_back(0x823c26);
			colors.push_back(0x8e4f3b);
			colors.push_back(0xca6748);
			colors.push_back(0xe38767);
			colors.push_back(0xe9a189);
			colors.push_back(0xffcc99);
			colors.push_back(0xffdfbf);
			colors.push_back(0x750d51);
			colors.push_back(0xc34b29);
			colors.push_back(0xd6a142);
			colors.push_back(0x008f3f);
			colors.push_back(0x2bb88a);
			colors.push_back(0x3144ad);
			colors.push_back(0x620d9b);
			colors.push_back(0xff4e6c);
			colors.push_back(0xb90027);
			colors.push_back(0xff6a29);
			colors.push_back(0xffd242);
			colors.push_back(0x50b700);
			colors.push_back(0x14ff88);
			colors.push_back(0x0a6cff);
			colors.push_back(0x8412d3);
			colors.push_back(0xd60096);
			colors.push_back(0xff0036);
			colors.push_back(0xff8000);
			colors.push_back(0xffec3d);
			colors.push_back(0x5ee52d);
			colors.push_back(0x8fffc7);
			colors.push_back(0x3688ff);
			colors.push_back(0xad14f9);
			colors.push_back(0xee00a8);
			colors.push_back(0xff7e7e);
			colors.push_back(0xffaa57);
			colors.push_back(0xffe888);
			colors.push_back(0x9bf75b);
			colors.push_back(0xccffd2);
			colors.push_back(0x9ad0ff);
			colors.push_back(0xd087ff);
			colors.push_back(0xf587d4);
			
			ofxControlColorSwatch *s = new ofxControlColorSwatch(colors, 0, 0);
			right_group->add(s);
			
			ofAddListener(s->colorChanged, this, &Editor::onColorChanged);
		}
		
		c.addSeparator();
		
		{
			ofxControlSliderI *o;
			
			o = c.addSliderI("R", 0, 255, 180 - 10);
			o->setValue(0);
			ofAddListener(o->valueChanged, this, &Editor::onColorSliderChanged);
			color_sliders[0] = o;
			
			o = c.addSliderI("G", 0, 255, 180 - 10);
			o->setValue(0);
			ofAddListener(o->valueChanged, this, &Editor::onColorSliderChanged);
			color_sliders[1] = o;
			
			o = c.addSliderI("B", 0, 255, 180 - 10);
			o->setValue(0);
			ofAddListener(o->valueChanged, this, &Editor::onColorSliderChanged);
			color_sliders[2] = o;
		}
		
		c.addSeparator();
		
		{
			ofxControlColorPicker *o;
			o = new ofxControlColorPicker(0, 0);
			right_group->add(o);
			
			ofAddListener(o->valueChanged, this, &Editor::onColorPickerChanged);
			
			picker = o;
		}
		
		c.end();
	}

	void setColor(ofColor c)
	{
		voxel_color.set(c);
		
		color_sliders[0]->setValue(c.r);
		color_sliders[1]->setValue(c.g);
		color_sliders[2]->setValue(c.b);
		
		picker->setValue(c.getHex());
		
		if (selected_voxel)
		{
			selected_voxel->color = c;
		}
	}
	
	string json_filename;

	void onSavePressed(ofEventArgs&)
	{
		ofFileDialogResult result = ofSystemSaveDialog(json_filename, "");
		if (result.bSuccess)
		{
			voxels.save(result.getPath());
		}
	}
	
	void onLoadPressed(ofEventArgs&)
	{
		ofFileDialogResult result = ofSystemLoadDialog();
		if (result.bSuccess)
		{
			string ext = ofFilePath::getFileExt(result.getName());
			bool loaded = false;
			
			if (ext == "json")
			{
				json_filename = result.getName();
				loaded = voxels.load(result.getPath());
			}
			
			if (!loaded)
			{
				ofSystemAlertDialog("Invalid file format");
			}
		}
	}
    
    void onLoadObjPressed(ofEventArgs&)
    {
        ofFileDialogResult result = ofSystemLoadDialog();
        if (result.bSuccess)
        {
            string ext = ofFilePath::getFileExt(result.getName());
            bool loaded = false;
            
            if (ext == "obj")
            {
                loaded = voxels.loadObj(result.getPath());
            }
            
            if (!loaded)
            {
                ofSystemAlertDialog("Invalid file format");
            }
        }

    }
	
	void onClear(ofEventArgs&)
	{
		voxels.clear();
		selected_voxel = NULL;
		half_selected_voxel = NULL;
	}
	
	void onUndo(ofEventArgs&)
	{
		if (!undo_buffer.empty())
		{
			Voxel data = undo_buffer.back();
			undo_buffer.pop_back();
			voxels = data;
		}
	}
	
	void onColorChanged(ofColor &color)
	{
		setColor(color);
	}

	void onColorSliderChanged(int &v)
	{
		ofColor c(color_sliders[0]->getValue(),
				  color_sliders[1]->getValue(),
				  color_sliders[2]->getValue());
		
		setColor(c);
	}
	
	void onColorPickerChanged(unsigned int &v)
	{
		setColor(ofColor::fromHex(v));
	}
	
	void onChangeTool(const void* sender, ofEventArgs&)
	{
		ofxControlButton *btn = (ofxControlButton*)sender;
		
		for (int i = 0; i < tool_group.size(); i++)
		{
			tool_group[i]->setValue(false);
		}
		
		string title = btn->getLabel();
		
		if (title == "PUT")
		{
			editmode = EDITMODE_PUT;
		}
		else if (title == "MOVE")
		{
			editmode = EDITMODE_MOVE;
		}
		else if (title == "RESIZE")
		{
			editmode = EDITMODE_RESIZE;
		}
		else if (title == "PICK COLOR")
		{
			editmode = EDITMODE_PICK_COLOR;
			
			if (selected_voxel)
			{
				setColor(selected_voxel->color);
			}
		}
	}

	deque<Voxel> undo_buffer;
	
	void pushUndoBuffer()
	{
		undo_buffer.push_back(voxels);
		
		if (undo_buffer.size() > 30)
		{
			undo_buffer.pop_front();
		}
	}
};

inline vector<Editor::Selection> Editor::pickup(int x, int y)
{
	const int BUFSIZE = 256;
	GLuint selectBuf[BUFSIZE];
	GLint hits;

	glPushAttrib(GL_ALL_ATTRIB_BITS);

	glSelectBuffer(BUFSIZE, selectBuf);
	glRenderMode(GL_SELECT);
	glMatrixMode(GL_PROJECTION);

	glPushMatrix();
	{
		glEnable(GL_DEPTH_TEST);

		glLoadIdentity();
		gluPickMatrix(x, viewport[3] - y, 1.0, 1.0, viewport);
		glMultMatrixd(projection);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMultMatrixd(modelview);

		glPushName(FLOOR_TAG);
		drawFloor();
		glPopName();

		glPushName(VOXEL_TAG);
		drawVoxel();
		glPopName();

		glDisable(GL_DEPTH_TEST);
		drawHandle();

		glMatrixMode(GL_PROJECTION);
	}
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);

	hits = glRenderMode(GL_RENDER);

	glPopAttrib();

	if (hits <= 0) return vector<Selection>();

	GLuint* ptr = selectBuf;

	vector<Selection> picked_stack;

	for (int i = 0; i < hits; i++)
	{
		GLuint num_names = ptr[0];
		GLfloat min_depth = (GLfloat)ptr[1] / 0xffffffff;
		GLfloat max_depth = (GLfloat)ptr[2] / 0xffffffff;

		GLuint* names = &ptr[3];

		Selection d;

		d.min_depth = min_depth;
		d.max_depth = max_depth;
		d.name_stack.insert(d.name_stack.begin(), names, (names + num_names));

		picked_stack.push_back(d);

		ptr += (3 + num_names);
	}

	sort(picked_stack.begin(), picked_stack.end(), Selection::sort_by_depth);

	return picked_stack;
}

inline vector<GLuint> Editor::hittest(int x, int y, unsigned int tag_name)
{
	vector<Selection> picked_stack = pickup(x, y);
	if (picked_stack.empty()) return vector<GLuint>();

	for (int i = 0; i < picked_stack.size(); i++)
	{
		const Selection& s = picked_stack[i];
		if (s.name_stack[0] == tag_name)
		{
			return s.name_stack;
		}
	}

	return vector<GLuint>();
}