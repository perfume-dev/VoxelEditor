#pragma once

#include "ofxJsonxx.h"
#include "ofxAssimpModelLoader.h"
#include "triboxoverlap.h"
#include <map>
#include <algorithm>

struct VoxelData
{
	int id;
	int x, y, z;
	int w, h, d;
	ofColor color;
	
	ofVec3f center() const
	{
		return ofVec3f(x + (float)w *0.5,
					   y + (float)h *0.5,
					   z + (float)d *0.5);
	}
	
	inline bool isInside(int xx, int yy, int zz) const
	{
		return xx >= x && xx < x + w
			&& yy >= y && yy < y + h
			&& zz >= z && zz < z + d;
	}
};

struct VoxelCoord {
    VoxelCoord(int nx, int ny, int nz) : x(nx), y(ny), z(nz) {};
    int x;
    int y;
    int z;
    bool operator==(const VoxelCoord &vc) const
    {
        return x == vc.x && y == vc.y && z == vc.z;
    }
    
    bool operator<(const VoxelCoord &vc) const
    {
        if (x < vc.x) {
            return true;
        } else if (x > vc.x) {
            return false;
        } else if (y < vc.y) {
            return true;
        } else if (y > vc.y) {
            return false;
        } else {
            return z < vc.z;
        }
    }
};

static bool compare_x(ofVec3f v1, ofVec3f v2) { return v1.x < v2.x; };
static bool compare_y(ofVec3f v1, ofVec3f v2) { return v1.y < v2.y; };
static bool compare_z(ofVec3f v1, ofVec3f v2) { return v1.z < v2.z; };

class Voxel
{
public:
	
	bool load(const string& path)
	{
		using namespace ofxJsonxx;
		
		Object json;
		if (loadFromFile(json, path) == false) return false;
		
		// check version string
		assert(get(json, "version", -1) == 1);
		get(json, "updatedAt", updatedAt);
		get(json, "metadata", metadata);

		Array voxels;
		assert(get(json, "voxels", voxels));
		
		this->voxels.clear();
		
		for (int i = 0; i < voxels.size(); i++)
		{
			const Object& voxel = voxels.get<Object>(i);
			VoxelData v;
			
			get(voxel, "id", v.id);
			get(voxel, "x", v.x);
			get(voxel, "y", v.y);
			get(voxel, "z", v.z);
			get(voxel, "w", v.w);
			get(voxel, "h", v.h);
			get(voxel, "d", v.d);
			
			voxel_ids.insert(v.id);
			
			int c;
			if (get(voxel, "color", c)) {
				v.color = ofColor::fromHex(c);
			}
			
			this->voxels.push_back(v);
		}
		
		return true;
	}
    
    bool intersect(ofVec3f face[3], float x, float y, float z, float length) {
        double box_center[3] = {x + length / 2, y + length / 2, z + length / 2};
        double box_half_size[3] = {length / 2, length / 2, length / 2};
        double triangle[3][3] = {{face[0].x, face[0].y, face[0].z}, {face[1].x, face[1].y, face[1].z}, {face[2].x, face[2].y, face[2].z}};
        return triBoxOverlap(box_center, box_half_size, triangle);
    }
    
    bool loadObj(const string& path)
    {
        ofxAssimpModelLoader model;

        // FIXME: workaround for assimp parsing bug.
        // assimp parseFile() has a bug that will crash if the last line of *.obj file is not a newline.
        // This hack copies the original *.obj file and then append a newline to it.
        string patched_path = path + ".patched.obj";
        ofFile::copyFromTo(path, patched_path);
        ofFile patched_file = ofFile(patched_path, ofFile::Append);
        patched_file << "\n";
        patched_file.close();
        
        // Load the patched file
        bool loaded = model.loadModel(patched_path);
        ofFile::removeFile(patched_path);
        
        if (loaded == false) return false;
        
        this->voxels.clear();
        map<VoxelCoord, ofColor> colors;
        
        for (int mesh_idx = 0; mesh_idx < model.getNumMeshes(); mesh_idx++) {
            ofMesh mesh = model.getMesh(mesh_idx);
            
            // Only support TRIANGLES
            if (mesh.getMode() != OF_PRIMITIVE_TRIANGLES) {
                ofLogError("VoxelData") << "loadObj(): not supported mesh mode: " << mesh.getMode();
                continue;
            }
            
            vector<ofVec3f> vertices = mesh.getVertices();
            
            // Calculate bounds
            float min_x = std::min_element(vertices.begin(), vertices.end(), compare_x)->x;
            float max_x = std::max_element(vertices.begin(), vertices.end(), compare_x)->x;
            float min_y = std::min_element(vertices.begin(), vertices.end(), compare_y)->y;
            float max_y = std::max_element(vertices.begin(), vertices.end(), compare_y)->y;
            float min_z = std::min_element(vertices.begin(), vertices.end(), compare_z)->z;
            float max_z = std::max_element(vertices.begin(), vertices.end(), compare_z)->z;

            // Calculate w, d, h
            float w = max_x - min_x;
            float d = max_z - min_z;
            float h = max_y - min_y;
            
            // Calculate step
            float step = (w * 60 < d * 80) ? d / 60 : w / 80;
            
            // Read texture
            ofTexture texture = model.getTextureForMesh(mesh_idx);
            ofPixels pixels;
            texture.readToPixels(pixels);
            
            // Iterate through all faces
            for (int idx = 0; idx < mesh.getNumIndices(); idx += 3) {
                ofVec3f face_vertices[3] = {vertices[mesh.getIndex(idx)], vertices[mesh.getIndex(idx + 1)], vertices[mesh.getIndex(idx + 2)]};
                std::vector<ofVec3f> face(&face_vertices[0], &face_vertices[0] + 3);
                
                // Calculate the bounding box of face
                float local_min_x = std::min_element(face.begin(), face.end(), compare_x)->x;
                float local_max_x = std::max_element(face.begin(), face.end(), compare_x)->x;
                float local_min_y = std::min_element(face.begin(), face.end(), compare_y)->y;
                float local_max_y = std::max_element(face.begin(), face.end(), compare_y)->y;
                float local_min_z = std::min_element(face.begin(), face.end(), compare_z)->z;
                float local_max_z = std::max_element(face.begin(), face.end(), compare_z)->z;
                
                // Calculate the voxel coordination
                int x_start = floor((local_min_x - min_x) / step);
                int x_end = ceil((local_max_x - min_x) / step);
                int y_start = floor((local_min_y - min_y) / step);
                int y_end = ceil((local_max_y - min_y) / step);
                int z_start = floor((local_min_z - min_z) / step);
                int z_end = ceil((local_max_z - min_z) / step);
                
                // Check all the voxels in the bounding box intersecting the face
                for (int x = x_start; x < x_end; x++)
                    for (int y = y_start; y < y_end; y++)
                        for (int z = z_start; z < z_end; z++) {
                            if (intersect(face_vertices, min_x + x * step, min_y + y * step, min_z + z * step, step) == false) continue;
                            
                            // Get the color of voxel (x, y, z). Currently it just pick color from one vertex.
                            // TODO: interpolate the color.
                            
                            ofColor color;
                            
                            if (mesh.hasColors() && mesh.getIndex(idx) < mesh.getNumColors()) {
                                color = mesh.getColor(mesh.getIndex(idx));
                            }
                            if (mesh.getIndex(idx) < mesh.getNumTexCoords()) {
                                ofVec2f texCoord = mesh.getTexCoord(mesh.getIndex(idx));
                                color = pixels.getColor(texCoord.x * pixels.getWidth(), texCoord.y * pixels.getHeight());
                            }
                            colors[VoxelCoord(x, y, z)] = color;
                        }
            }
        }
        
        // Add the voxels
        int id = 0;
        for (auto record : colors) {
            VoxelData v;
            v.x = record.first.x;
            v.y = record.first.y;
            v.z = record.first.z;
            v.color = record.second;
            v.w = v.d = v.h = 1;
            v.id = id++;
            this->voxels.push_back(v);
            this->voxel_ids.insert(v.id);
        }
        
        return true;
    }
	
	bool save(const string& path)
	{
		using namespace ofxJsonxx;
		
		Object json;
		ofxJsonxx::set(json, "version", 1);
		ofxJsonxx::set(json, "updatedAt", time(0));
		ofxJsonxx::set(json, "metadata", metadata);
		
		Array voxels;
		
		for (int i = 0; i < this->voxels.size(); i++)
		{
			const VoxelData& v = this->voxels[i];
			Object voxel;
			
			ofxJsonxx::set(voxel, "id", i);
			ofxJsonxx::set(voxel, "x", v.x);
			ofxJsonxx::set(voxel, "y", v.y);
			ofxJsonxx::set(voxel, "z", v.z);
			ofxJsonxx::set(voxel, "w", v.w);
			ofxJsonxx::set(voxel, "h", v.h);
			ofxJsonxx::set(voxel, "d", v.d);
			ofxJsonxx::set(voxel, "color", v.color.getHex());
			
			voxels << voxel;
		}
		
		json << "voxels" << voxels;
		
		return saveToFile(json, path);
	}
	
	bool exists(const ofVec3f& pos)
	{
		vector<VoxelData>::iterator it = voxels.begin();
		while (it != voxels.end())
		{
			const VoxelData& voxel = *it;
			if (voxel.x == pos.x
				&& voxel.y == pos.y
				&& voxel.z == pos.z)
			{
				return true;
			}
			
			it++;
		}
		
		return false;
	}
	
	void remove(const ofVec3f& pos)
	{
		vector<VoxelData>::iterator it = voxels.begin();
		while (it != voxels.end())
		{
			const VoxelData& voxel = *it;
			if (voxel.x == pos.x
				&& voxel.y == pos.y
				&& voxel.z == pos.z)
			{
				voxel_ids.erase(it->id);
				it = voxels.erase(it);
			} else it++;
		}
	}
	
	void remove(VoxelData& voxel)
	{
		int id = voxel.id;
		
		vector<VoxelData>::iterator it = voxels.begin();
		while (it != voxels.end())
		{
			VoxelData &v = *it;
			if (v.id == id)
			{
				voxel_ids.erase(v.id);
				it = voxels.erase(it);
			}
			else it++;
		}
	}
	
	void add(const VoxelData& v)
	{
		remove(ofVec3f(v.x, v.y, v.z));
		voxels.push_back(v);
		
		if (voxel_ids.empty() == false)
		{
			voxels.back().id = *voxel_ids.rbegin() + 1;
			voxel_ids.insert(0);
		}
		
		voxel_ids.insert(voxels.back().id);
	}
	
	void clear()
	{
		voxels.clear();
	}
	
	vector<VoxelData>& getVoxels() { return voxels; }
	
private:
	
	int updatedAt;
	string metadata;
	set<int> voxel_ids;
	vector<VoxelData> voxels;
};