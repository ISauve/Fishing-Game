#include "Object.hpp"
#include "Scene.hpp"
#include <iostream>
#include <vector>
#include <string>

using namespace std;
using namespace glm;

Mesh::Mesh(ShaderProgram* shader, Scene* scene, aiMesh* mesh, const aiScene* aiscene, string texturePrefix) : Object(shader, scene)
{
    // VAO is already bound
    m_shader->enable();
    
    // Initialize our bounding box bounds
    m_minBounds = vec3(0.0f);
    m_maxBounds = vec3(0.0f);
    
    // Copy vertex data from assimp mesh
    struct VertexData {
        vec3 position;
        vec3 normal;
        vec2 texCoords;
    };
    VertexData* vertexData = new VertexData[ mesh->mNumVertices ];
    for (int i=0; i < mesh->mNumVertices; i++)
    {
        VertexData vtx;
        vtx.position      = vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        vtx.normal        = vec3(mesh->mNormals[i].x,  mesh->mNormals[i].y,  mesh->mNormals[i].z);
        if (mesh->mTextureCoords[0])
            vtx.texCoords = vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        else
            vtx.texCoords = vec2(0.0f);
        
        vertexData[i] = vtx;
        
        // Update bounding box
        if (vtx.position.x < m_minBounds.x) m_minBounds.x = vtx.position.x;
        if (vtx.position.x > m_maxBounds.x) m_maxBounds.x = vtx.position.x;

        if (vtx.position.y < m_minBounds.y) m_minBounds.y = vtx.position.y;
        if (vtx.position.y > m_maxBounds.y) m_maxBounds.y = vtx.position.y;

        if (vtx.position.z < m_minBounds.z) m_minBounds.z = vtx.position.z;
        if (vtx.position.z > m_maxBounds.z) m_maxBounds.z = vtx.position.z;
    }
    // Store to VBO - note that this is possible bc the struct's memory layout is sequential
    glGenBuffers(1, &m_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VertexData) * mesh->mNumVertices, vertexData, GL_STATIC_DRAW);
    
    // Copy index data from assimp mesh
    vector<GLuint> indices;
    for (int i=0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (int j=0; j < face.mNumIndices; j++)
            indices.push_back(face.mIndices[j]);
    }
    m_numIndices = (int) indices.size();
    m_ebo = storeToEBO(&indices[0], (int) indices.size() * sizeof(GLuint));
    
    // Load the 3 associated textures into texture units 0-2
    glActiveTexture(GL_TEXTURE0);
    m_textureIDs.push_back( storeTex(texturePrefix + "_diffuse.png") );
    
    /*
     Note: disabling specular & normal mapping because not all the objects have these & the objects
     that do have them look just fine without it
     */
//    glActiveTexture(GL_TEXTURE1);
//    m_textureIDs.push_back( storeTex(texturePrefix + "_specular.png") );
//
//    glActiveTexture(GL_TEXTURE2);
//    m_textureIDs.push_back( storeTex(texturePrefix + "_normal.png") );
    
    // Tell OpenGL where to find/how to interpret...
    //      1) The vertex positions
    GLint location = m_shader->getAttribLocation("position");
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);
    glEnableVertexAttribArray(location);
    
    //      2) The vertex normals
    location = m_shader->getAttribLocation("normal");
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, normal));
    glEnableVertexAttribArray(location);
    
    //      3) The texture coordinates
    location = m_shader->getAttribLocation("textureCoords");
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, texCoords));
    glEnableVertexAttribArray(location);
    
    //      4) The diffuse texture uniform
    glActiveTexture(GL_TEXTURE0);
    location = m_shader->getUniformLocation("DiffuseTexture");
    glUniform1i(location, 0);   // texture unit 0
    
//    //      5) The specular texture uniform
//    glActiveTexture(GL_TEXTURE1);
//    location = m_shader->getUniformLocation("SpecularTexture");
//    glUniform1i(location, 1);   // texture unit 1
//
//    //      6) The normal map uniform
//    glActiveTexture(GL_TEXTURE2);
//    location = m_shader->getUniformLocation("NormalMap");
//    glUniform1i(location, 2);   // texture unit 2
    
    m_shader->disable();
    glBindVertexArray( 0 );
    releaseData();
    
    initBoundingBoxData();
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &bb_vao);
    glDeleteBuffers(1, &bb_vbo);
}


void Mesh::uploadCustomUniforms(Mode m)
{
    GLint location = m_shader->getUniformLocation("IsTerrainObject");
    glUniform1i(location, false);
    
    location = m_shader->getUniformLocation("IsMeshObject");
    glUniform1i(location, true);
}

void Mesh::bindData()
{
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureIDs[0]);

//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, m_textureIDs[1]);
//
//    glActiveTexture(GL_TEXTURE2);
//    glBindTexture(GL_TEXTURE_2D, m_textureIDs[2]);

    CHECK_GL_ERRORS;
}

void Mesh::drawElements()
{
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
    CHECK_GL_ERRORS;
}

void Mesh::releaseData()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
//    glActiveTexture(GL_TEXTURE1);
//    glBindTexture(GL_TEXTURE_2D, 0);
//
//    glActiveTexture(GL_TEXTURE2);
//    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    
    CHECK_GL_ERRORS;
}

void Mesh::renderToShadowMap()
{
    // Minimalistic rendering: we only need to bind the position data & model matrix, then render
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    
    struct VertexData {
        vec3 position;
        vec3 normal;
        vec2 texCoords;
    };
    GLint location = m_scene->shadowShader()->getAttribLocation("position");
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)0);
    glEnableVertexAttribArray(location);
    
    mat4 model = modelMatrix();
    location = m_scene->shadowShader()->getUniformLocation("Model");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(model));
    
    drawElements();
    
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    CHECK_GL_ERRORS;
}


/***********************************************************
                      Bounding Box
 ***********************************************************/

void Mesh::initBoundingBoxData()
{
    // Load bounding box data
    glGenVertexArrays(1, &bb_vao);
    glBindVertexArray(bb_vao);
    
    bb_shader.generateProgramObject();
    bb_shader.attachVertexShader( string("Assets/Shaders/BBVtxShader.vs").c_str() );
    bb_shader.attachFragmentShader( string("Assets/Shaders/BBFragShader.fs").c_str() );
    bb_shader.link();
    
    bb_shader.enable();
    
    GLfloat vertices[] = {
        m_maxBounds.x, m_minBounds.y, m_minBounds.z,   // Back face
        m_maxBounds.x, m_maxBounds.y, m_minBounds.z,
        m_minBounds.x, m_minBounds.y, m_minBounds.z,
        m_maxBounds.x, m_maxBounds.y, m_minBounds.z,
        m_minBounds.x, m_minBounds.y, m_minBounds.z,
        m_minBounds.x, m_maxBounds.y, m_minBounds.z,
        
        m_minBounds.x, m_minBounds.y, m_maxBounds.z,   // Front face
        m_minBounds.x, m_maxBounds.y, m_maxBounds.z,
        m_maxBounds.x, m_minBounds.y, m_maxBounds.z,
        m_minBounds.x, m_maxBounds.y, m_maxBounds.z,
        m_maxBounds.x, m_minBounds.y, m_maxBounds.z,
        m_maxBounds.x, m_maxBounds.y, m_maxBounds.z,
        
        m_minBounds.x, m_minBounds.y, m_minBounds.z,   // Left face
        m_minBounds.x, m_maxBounds.y, m_minBounds.z,
        m_minBounds.x, m_minBounds.y, m_maxBounds.z,
        m_minBounds.x, m_maxBounds.y, m_minBounds.z,
        m_minBounds.x, m_minBounds.y, m_maxBounds.z,
        m_minBounds.x, m_maxBounds.y, m_maxBounds.z,
        
        m_maxBounds.x, m_minBounds.y, m_maxBounds.z,    // Right face
        m_maxBounds.x, m_maxBounds.y, m_maxBounds.z,
        m_maxBounds.x, m_minBounds.y, m_minBounds.z,
        m_maxBounds.x, m_maxBounds.y, m_maxBounds.z,
        m_maxBounds.x, m_minBounds.y, m_minBounds.z,
        m_maxBounds.x, m_maxBounds.y, m_minBounds.z,
        
        m_maxBounds.x, m_minBounds.y, m_maxBounds.z,     // Bottom face
        m_maxBounds.x, m_minBounds.y, m_minBounds.z,
        m_minBounds.x, m_minBounds.y, m_maxBounds.z,
        m_maxBounds.x, m_minBounds.y, m_minBounds.z,
        m_minBounds.x, m_minBounds.y, m_maxBounds.z,
        m_minBounds.x, m_minBounds.y, m_minBounds.z,
        
        m_minBounds.x, m_maxBounds.y, m_maxBounds.z,    // Top face
        m_minBounds.x, m_maxBounds.y, m_minBounds.z,
        m_maxBounds.x, m_maxBounds.y, m_maxBounds.z,
        m_minBounds.x, m_maxBounds.y, m_minBounds.z,
        m_maxBounds.x, m_maxBounds.y, m_maxBounds.z,
        m_maxBounds.x, m_maxBounds.y, m_minBounds.z,
    };
    bb_vbo = storeToVBO(vertices, sizeof(vertices));
    
    GLint location = bb_shader.getAttribLocation("position");
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    
    bb_shader.disable();
    glBindVertexArray(0);
}

void Mesh::renderBoundingBox(Mode mode)
{
    glBindVertexArray(bb_vao);
    bb_shader.enable();
    glBindBuffer( GL_ARRAY_BUFFER, bb_vbo );
    
    mat4 model = modelMatrix();
    GLint location = bb_shader.getUniformLocation("Model");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(model));
    
    mat4 view = m_scene->camera()->viewMatrix();
    location = bb_shader.getUniformLocation("View");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(view));

    mat4 projection = m_scene->camera()->projMatrix();
    location = bb_shader.getUniformLocation("Projection");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(projection));
    
    bool clippingEnabled = mode == REFLECTION || mode == REFRACTION;
    location = bb_shader.getUniformLocation("ClippingEnabled");
    glUniform1f(location, clippingEnabled);
    
    if (clippingEnabled)
    {
        vec4 clippingPlane;
        if (mode == REFLECTION)
            clippingPlane = vec4(0, 1, 0, m_scene->water()->position().y);
        else if (mode == REFRACTION)
            clippingPlane = vec4(0, -1, 0, m_scene->water()->position().y + 1.0f);
        
        location = bb_shader.getUniformLocation("ClippingPlane");
        glUniform4fv(location, 1, value_ptr(clippingPlane));
    }
    CHECK_GL_ERRORS;
    
    glDrawArrays(GL_TRIANGLES, 0, 36);
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    bb_shader.disable();
    glBindVertexArray(0);
    
    CHECK_GL_ERRORS;
}

bool Mesh::collision(Mesh* m)
{
    // Find the "real" locations of the bounding boxes by transforming then with the model matrix
    vec3 min1 = vec3(modelMatrix() * vec4(m_minBounds, 1.0f));
    vec3 max1 = vec3(modelMatrix() * vec4(m_maxBounds, 1.0f));
    
    vec3 min2 = vec3(m->modelMatrix() * vec4(m->m_minBounds, 1.0f));
    vec3 max2 = vec3(m->modelMatrix() * vec4(m->m_maxBounds, 1.0f));
    
    // We're going to simplify our calculations by only considering collisions in the X & Z axis
    // (since fish exists on more or less the same Y plane & our "fishing line" descends to infinite depth)
    
    /*
     Separating Axis Theorem: for 2 oriented bounding boxes to be disjoint, there must be some axis
     along which their projections are disjoint. For the 2D case, we consider the orthogonal edges of
     each bounding box.
     */
    
    // Step 1) Define the 4 corners of our bounding boxes
    vec2 corners1[4] = {
        vec2(min1.x, min1.z),   // bottom left
        vec2(max1.x, min1.z),   // bottom right
        vec2(max1.x, max1.z),   // top right
        vec2(min1.x, max1.z)    // top left
    };
    
    vec2 corners2[4] = {
        vec2(min2.x, min2.z),
        vec2(max2.x, min2.z),
        vec2(max2.x, max2.z),
        vec2(min2.x, max2.z)
    };
    
    {
        // Step 2) Find the x & z axes of our oriented bounding box
        vec2 axes[2] {
            corners1[1] - corners1[0], // X axis
            corners1[3] - corners1[0]  // Z axis
        };
        
        // Step 3) Make each axis length = 1 / axis length
        axes[0] /= length(axes[0]) * length(axes[0]);
        axes[1] /= length(axes[1]) * length(axes[1]);
        
        // Step 4) Get the projection of the "origin" corner (corner 0) onto each axis
        float originProjections[2] {
            dot(corners1[0], axes[0]),
            dot(corners1[0], axes[1])
        };
        
        // For each axis...
        for (int i=0; i<2; i++) {
            
            // Step 5) Project the other box's corners onto this axis
            float tMin = numeric_limits<float>::infinity();
            float tMax = - numeric_limits<float>::infinity();
            
            for (int j=0; j<4; j++) {
                float t = dot(corners2[j], axes[i]);
                
                tMin = (t < tMin) ? t : tMin;
                tMax = (t > tMax) ? t : tMax;
            }
            
            // Step 6) See if [tMin, tMax] intersects [0, 1] along this axis (from the "origin")
            if ( (tMin > (originProjections[i] + 1.0f)) || (tMax < originProjections[i]) )
                return false;   // No intersection -> not possible for the boxes to overlap
        }
    }
    
    // Repeat steps 2-6, but this time project "our" box onto the "other"
    {
        vec2 axes[2] {
            corners2[1] - corners2[0],
            corners2[3] - corners2[0]
        };
        axes[0] /= length(axes[0]) * length(axes[0]);
        axes[1] /= length(axes[1]) * length(axes[1]);
        
        float originProjections[2] {
            dot(corners2[0], axes[0]),
            dot(corners2[0], axes[1])
        };
        
        for (int i=0; i<2; i++) {
            float tMin = numeric_limits<float>::infinity();
            float tMax = - numeric_limits<float>::infinity();
            for (int j=0; j<4; j++) {
                float t = dot(corners1[j], axes[i]);
                tMin = (t < tMin) ? t : tMin;
                tMax = (t > tMax) ? t : tMax;
            }
            
            if ( (tMin > (originProjections[i] + 1.0f)) || (tMax < originProjections[i]) )
                return false;
        }
    }
    
    // If we get to here, then that means that there was intersections along all 4 axes
    // Therefore, the bounding boxes overlap -> we have a collision
    return true;
}
