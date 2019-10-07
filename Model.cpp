#include "Model.hpp"

using namespace std;
using namespace glm;

//#define DEBUG_PRINT

Model::Model(ShaderProgram* shader, Scene* scene, string path) : Renderable(),
    m_scene(scene), m_position(vec3(0.0)), m_facing(vec3(0.0f, 0.0f, -1.0f))
{
#ifdef DEBUG_PRINT
    cout << "Loading data for model: " << path << endl;
#endif
    // Load the model into an assimp scene object
    Assimp::Importer importer;
    const aiScene* aiscene = importer.ReadFile(path, aiProcess_Triangulate);
    
    if (!aiscene || aiscene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !aiscene->mRootNode) {
        cout << "Error loading model at " << path << ": " << importer.GetErrorString() << endl;
        return;
    }
    
    string texturefolder = path.substr(0, path.find_last_of('/') + 1);
    generateMeshesRecursively(shader, scene, aiscene->mRootNode, aiscene, texturefolder);
}

void Model::generateMeshesRecursively(ShaderProgram* shader, Scene* scene, aiNode* node, const aiScene* aiscene, string texturefolder)
{
    // Process all the meshes
    for (int i=0; i < node->mNumMeshes; i++) {
        
        aiMesh* mesh = aiscene->mMeshes[node->mMeshes[i]];
#ifdef DEBUG_PRINT
        cout << "\tCreating mesh: " << mesh->mName.C_Str() << endl;
#endif
        string texturePrefix = texturefolder + string(mesh->mName.C_Str());
        m_modelMeshes.push_back(new Mesh(shader, scene, mesh, aiscene, texturePrefix));
    }
    
    // Recurse on the node's children
    for (int i=0; i < node->mNumChildren; i++)
        generateMeshesRecursively(shader, scene, node->mChildren[i], aiscene, texturefolder);
}

Model::~Model()
{
    for (Mesh* mesh : m_modelMeshes) {
        delete mesh;
    }
}

void Model::render(Mode m)
{
    for (Mesh* mesh : m_modelMeshes)
        mesh->render(m);
}

void Model::renderToShadowMap()
{
    for (Mesh* mesh : m_modelMeshes)
        mesh->renderToShadowMap();
}

void Model::renderBoundingBox(Mode m)
{
    for (Mesh* mesh : m_modelMeshes)
        mesh->renderBoundingBox(m);
}

bool Model::collision(Model* m)
{
    for (auto mesh1 : m_modelMeshes) {
        for (auto mesh2 : m->m_modelMeshes) {
            if ( mesh1->collision(mesh2) ) return true;
        }
    }
    
    return false;
}

void Model::setPosition(glm::vec3 p)
{
    m_position = p;
    for (Mesh* mesh : m_modelMeshes)
        mesh->setPosition(p);
}

void Model::setSize(float s)
{
    for (Mesh* mesh : m_modelMeshes)
        mesh->setSize(s);
}
