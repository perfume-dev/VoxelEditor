#include "ofMain.h"

#include "ofxModifierKeys.h"
#include "ControlOF.h"

#include "VoxelData.h"
#include "Editor.h"

class ofApp : public ofBaseApp
{
public:
	
	Editor editor;
	
	void setup()
	{
		ofSetFrameRate(60);
		ofSetVerticalSync(true);
		ofBackground(127);
		
		ofxControlWidget::defaultTextColor = ofColor(255);
		ofxControlWidget::defaultForegroundColor = ofColor(0x555555);
		ofxControlWidget::defaultBackgroundColor = ofColor(0);

		editor.setup();
	}
	
	void update()
	{
		editor.update();
	}
	
	void draw()
	{
		editor.draw();
	}

	void keyPressed(int key)
	{
		if (key == 'a')
		{
			editor.moveCamera(22.5, 0);
		}
		else if (key == 'd')
		{
			editor.moveCamera(-22.5, 0);
		}
		else if (key == 'w')
		{
			editor.moveCamera(0, 0, -10);
		}
		else if (key == 's')
		{
			editor.moveCamera(0, 0, 10);
		}
		
		if (key == 'q')
		{
			editor.moveCamera(0, 10);
		}
		if (key == 'e')
		{
			editor.moveCamera(0, -10);
		}
		
		if (key == '1')
		{
			editor.setEditMode(Editor::EDITMODE_PUT);
		}
		else if (key == '2')
		{
			editor.setEditMode(Editor::EDITMODE_MOVE);
		}
		else if (key == '3')
		{
			editor.setEditMode(Editor::EDITMODE_RESIZE);
		}
		else if (key == '4')
		{
			editor.setEditMode(Editor::EDITMODE_PICK_COLOR);
		}
		
		if (key == ' ')
		{
			editor.put();
		}
		else if (key == OF_KEY_BACKSPACE || key == OF_KEY_DEL)
		{
			editor.remove();
		}
		
		if (key == OF_KEY_LEFT)
		{
			editor.moveCursor(-1, 0);
		}
		else if (key == OF_KEY_RIGHT)
		{
			editor.moveCursor(1, 0);
		}
		else if (key == OF_KEY_UP)
		{
			if (ofGetModifierPressed(OF_KEY_SHIFT))
				editor.moveCursor(0, 0, -1);
			else
				editor.moveCursor(0, 1);
		}
		else if (key == OF_KEY_DOWN)
		{
			if (ofGetModifierPressed(OF_KEY_SHIFT))
				editor.moveCursor(0, 0, 1);
			else
				editor.moveCursor(0, -1);
		}
	}

	void keyReleased(int key)
	{
	}
	
	void mouseMoved(int x, int y)
	{
		if (ofxControl::hasFocus()) return;
		
		editor.onMoved(x, y);
	}

	void mouseDragged(int x, int y, int button)
	{
		if (ofxControl::hasFocus()) return;
		
		float dx = ofGetPreviousMouseX() - x;
		float dy = ofGetPreviousMouseY() - y;
		
		editor.moveCamera(dx * 0.5, dy * 0.5);
	}
	
	void mousePressed(int x, int y, int button)
	{
		if (ofxControl::hasFocus()) return;
		
		static float last_click_time = 0;
		if ((ofGetElapsedTimef() - last_click_time) < 0.25)
		{
			editor.onDoubleClick(x, y);
		}
		else
		{
			editor.onPressed(x, y);
		}
		
		last_click_time = ofGetElapsedTimef();
	}

	void mouseReleased(int x, int y, int button)
	{
		if (ofxControl::hasFocus()) return;
		
		editor.onReleased(x, y);
	}
	
	void windowResized(int w, int h)
	{
		editor.onResized(w, h);
	}
};


int main(int argc, const char** argv)
{
	ofSetupOpenGL(1280, 720, OF_WINDOW);
	ofRunApp(new ofApp);
	return 0;
}
