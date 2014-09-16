#pragma once

#include "ofxJsonxx.h"

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