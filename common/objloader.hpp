#ifndef OBJLOADER_H
#define OBJLOADER_H

bool loadOBJ_tris(
	const char * path,
	std::vector<glm::vec3> & out_vertices,
	std::vector<glm::vec3> & out_normals,
	std::vector<glm::vec2> & out_uvs
);

bool loadOBJ(
	const char * path, 
	std::vector<glm::vec3> & out_vertices, 
	std::vector<glm::vec3> & out_normals,
	std::vector<glm::vec2> & out_uvs
);



bool loadAssImp(
	const char * path, 
	std::vector<unsigned short> & indices,
	std::vector<glm::vec3> & vertices,
	std::vector<glm::vec3> & normals
);

#endif