#include "Model.hpp"
#include "Scene.hpp"
#include <string>

using namespace std;
using namespace glm;

TerrainObject::TerrainObject(ShaderProgram* shader, Scene* scene, string path) : Model(shader, scene, path)
{
}

void TerrainObject::setOnTerrain(float x, float z)
{
    m_position = vec3(x, m_scene->terrain()->getHeightAt(x, z), z);
    for (Mesh* mesh : m_modelMeshes)
        mesh->setPosition(m_position);
}

