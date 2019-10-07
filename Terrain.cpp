#include "Object.hpp"
#include "Scene.hpp"
#include <string>
#include "lodepng/lodepng.h"

using namespace std;
using namespace glm;

Terrain::Terrain(ShaderProgram* shader, Scene* scene, float size, float max) : Object(shader, scene),
    m_size(size), m_maxHeight(max)
{
    // VAO is already bound
    m_shader->enable();
    
    // Load the height map image
    unsigned char* heightMap;
    unsigned height;
    string filepath = "Assets/Terrain/heightmap.png";
    
    unsigned error = lodepng_decode32_file(&heightMap, &m_heightMapSize, &height, filepath.c_str());
    if (error) {
        cerr << "Error decoding heightmap. " << error << ": " << lodepng_error_text(error) << endl;
        return;
    } else if (height != m_heightMapSize) {
        cerr << "Error decoding heightmap. Mismatched width and height:" << m_heightMapSize << ", " << height << endl;
        return;
    }
    calculateHeightsAndNormals(heightMap);
    
    // Dynamically generate a large square made up of triangles with m_heightMapSize vertices along each edge
    // Note that the square we will generate has its TOP LEFT CORNER at (0,0,0)
    int totalVtcs = m_heightMapSize * m_heightMapSize;
    GLfloat* positions = new GLfloat[totalVtcs * 3];
    GLfloat* normals = new GLfloat[totalVtcs * 3];
    GLfloat* textureCoords = new GLfloat[totalVtcs * 2];
    int count = 0;
    for (int i=0; i<m_heightMapSize; i++) {
        for (int j=0; j<m_heightMapSize; j++) {
            float sideLength = m_size / ((float) m_heightMapSize - 1);
            
            GLfloat x = (float) i * sideLength;
            GLfloat z = (float) j * sideLength;
            positions[count*3] = x;
            positions[count*3+1] = m_heights[i][j];
            positions[count*3+2] = z;
            
            vec3 norm = m_normals[i][j];
            normals[count*3] = norm.x;
            normals[count*3+1] = norm.y;
            normals[count*3+2] = norm.z;
            
            /* Generate the texture coordinates */
            
            // "Shrink" the displayed texture so that it repeats instead of being 1 large texture
            // and so that it always looks about the same, regardless of how large we make the terrain
            float shrinkFactor = m_size / 50.0f;
            
            GLfloat xT = (float) j /  ((float) m_heightMapSize - 1);
            GLfloat yT = (float) i / ((float) m_heightMapSize - 1);
            textureCoords[count*2] = xT * shrinkFactor;
            textureCoords[count*2+1] = yT * shrinkFactor;
            
            count++;
        }
    }
    m_vbo = storeToVBO(positions, sizeof(GLfloat) * totalVtcs * 3,
                       normals, sizeof(GLfloat) * totalVtcs * 3,
                       textureCoords, sizeof(GLfloat) * totalVtcs * 2);

    // Generate the indices for drawing these triangles
    m_numIndices = 6 * (m_heightMapSize-1) * (m_heightMapSize -1);
    GLuint* indices = new GLuint[m_numIndices];
    count = 0;
    for (int i=0; i<m_heightMapSize-1; i++) {
        for (int j=0; j<m_heightMapSize-1; j++) {
            // Make a square out of 2 triangles
            GLuint topLeft = i * m_heightMapSize + j;  // adding m_heightMapSize skips to "next row"
            GLuint topRight = topLeft + 1;          // adding 1 skips to "next column"
            GLuint bottomLeft = (i+1) * m_heightMapSize + j;
            GLuint bottomRight = bottomLeft + 1;
            
            indices[count++] = topLeft;
            indices[count++] = bottomLeft;
            indices[count++] = topRight;
            
            indices[count++] = topRight;
            indices[count++] = bottomLeft;
            indices[count++] = bottomRight;
        }
    }
    m_ebo = storeToEBO(indices, sizeof(GLuint) * m_numIndices);
    
    // Load the grass image into texture unit 0
    glActiveTexture(GL_TEXTURE0);
    m_textureIDs.push_back( storeTex("Assets/Terrain/grass.png", GL_REPEAT) );
    
    // Load the dirt image into texture unit 1
    glActiveTexture(GL_TEXTURE1);
    m_textureIDs.push_back( storeTex("Assets/Terrain/dirt.png", GL_REPEAT) );
    
    // Tell OpenGL where to find/how to interpret...
    //      1) The vertex positions
    GLint location = m_shader->getAttribLocation("position");
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    
    //      2) The vertex normals
    location = m_shader->getAttribLocation("normal");
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(GLfloat) * totalVtcs * 3));
    glEnableVertexAttribArray(location);
    
    //      3)  The texture coordinates
    location = m_shader->getAttribLocation("textureCoords");
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, 0, (void*)(sizeof(GLfloat) * totalVtcs * 6));
    glEnableVertexAttribArray(location);
    
    //      4) The grass texture uniform
    glActiveTexture(GL_TEXTURE0);
    location = m_shader->getUniformLocation("GrassTexture");
    glUniform1i(location, 0);   // texture unit 0
    
    //      5) The dirt texture uniform
    glActiveTexture(GL_TEXTURE1);
    location = m_shader->getUniformLocation("DirtTexture");
    glUniform1i(location, 1);   // texture unit 1
    
    //     6) The shadow map texture uniform
    glActiveTexture(GL_TEXTURE2);
    location = m_shader->getUniformLocation("ShadowMap");
    glUniform1i(location, 2);   // texture unit 2

    uploadMaterialUniforms(vec3(1.0, 1.0, 1.0), // kd
                           vec3(0.1, 0.1, 0.1), // ks - very little specular lighting for terrain
                           32);                 // shininess
    
    free(heightMap);
    m_shader->disable();
    glBindVertexArray(0);
    releaseData();
};

void Terrain::calculateHeightsAndNormals(unsigned char* heightMap)
{
    // Initialize our matrices
    m_heights.resize(m_heightMapSize);
    m_normals.resize(m_heightMapSize);
    for (int i=0; i<m_heightMapSize; i++) {
        m_heights[i].resize(m_heightMapSize);
        m_normals[i].resize(m_heightMapSize);
    }
    
    // Calculate the heights and normals for each pixel of this map
    for (int i=0; i<m_heightMapSize; i++) {
        for (int j=0; j<m_heightMapSize; j++) {
            // Image is in 32-bit RGBA format -> rows of size m_heightMapSize * 4
            int nextPixelIndex = (i * m_heightMapSize * 4) + (j * 4);
            float rawHeight = heightMap[nextPixelIndex];  // read the R value to get height
            float height = (rawHeight - 128) / 128;         // Get the range to be (-1)-1
            m_heights[i][j] =  height * m_maxHeight;
        }
    }
    for (int i=0; i<m_heightMapSize; i++) {
        for (int j=0; j<m_heightMapSize; j++) {
            // Conditionals are to make sure we don't index out of bounds
            float heightL = (i==0) ? m_heights[0][j] : m_heights[i-1][j];
            float heightR = (i==m_heightMapSize-1) ? m_heights[0][j] : m_heights[i+1][j];
            float heightU = (j==0) ? m_heights[i][0] : m_heights[i][j-1];
            float heightD = (j==m_heightMapSize-1) ? m_heights[i][0] : m_heights[i][j+1];
            vec3 normal = vec3(heightL - heightR, 1.0f, heightU - heightD);
            m_normals[i][j] = normalize(normal);
        }
    }
}

// Use barycentric coordinates to determine the height at point pos given 3 vertices p1/p2/p3
float Terrain::baryCentric(vec3 p1, vec3 p2, vec3 p3, vec2 pos)
{
    float det = (p2.z - p3.z) * (p1.x - p3.x) + (p3.x - p2.x) * (p1.z - p3.z);
    float l1 = ((p2.z - p3.z) * (pos.x - p3.x) + (p3.x - p2.x) * (pos.y - p3.z)) / det;
    float l2 = ((p3.z - p1.z) * (pos.x - p3.x) + (p1.x - p3.x) * (pos.y - p3.z)) / det;
    float l3 = 1.0f - l1 - l2;
    return l1 * p1.y + l2 * p2.y + l3 * p3.y;
}

// Returns terrain height at point worldX, worldZ
float Terrain::getHeightAt(float worldX, float worldZ)
{
    // Map these x and z coordinates to coords relative to terrain
    float terrainX = worldX - m_position.x;
    float terrainZ = worldZ - m_position.z;
    
    // Terrain is just a grid of squares - find which square this terrain coord is in
    float gridSqSz = m_size / float(m_heights.size());
    int gridX = floor(terrainX / gridSqSz);    // takes the floor
    int gridZ = floor(terrainZ / gridSqSz);
    
    if (gridX < 0 || gridZ < 0 || gridX >= m_heights.size()-1 || gridZ >= m_heights.size()-1)
        return 0;
    
    // Once scaled to "real size", a terrain grid can actually be pretty big - so we'll
    // calculate our relative position within this grid square, use that to figure out which
    // triangle we are standing in, & then use barycentric coordinates to find the height
    // at that exact spot
    float xCoord = std::fmod(terrainX, float(gridSqSz)) / gridSqSz;     // range 0-1
    float zCoord = std::fmod(terrainZ, float(gridSqSz)) / gridSqSz;     // range 0-1
    float preciseHeight;
    
    if (xCoord <= (1-zCoord)) {   // we're in the "top left" triangle of the grid square
        preciseHeight = baryCentric(vec3(0, m_heights[gridX][gridZ], 0),     // top left grid vertex
                                    vec3(1, m_heights[gridX+1][gridZ], 0),   // top right grid vertex
                                    vec3(0, m_heights[gridX][gridZ+1], 1),   // bottom left grid vertex
                                    vec2(xCoord, zCoord));                  // our position
    }
    else {    // "bottom left" triangle
        preciseHeight = baryCentric(vec3(1, m_heights[gridX+1][gridZ], 0),     // top right grid vertex
                                    vec3(1, m_heights[gridX+1][gridZ+1], 1),   // bottom right grid vertex
                                    vec3(0, m_heights[gridX][gridZ+1], 1),     // bottom left grid vertex
                                    vec2(xCoord, zCoord));                    // our position
    }
    
    // Precise height is the height of the height map at this point - we need to add the Y component
    // of our position to this in order to account for the offset it creates
    return preciseHeight + m_position.y;
}

// Rendering ----------------------------------------------------------------------------------------

void Terrain::uploadCustomUniforms(Mode m)
{
    GLint location = m_shader->getUniformLocation("IsTerrainObject");
    glUniform1i(location, true);
    
    location = m_shader->getUniformLocation("IsMeshObject");
    glUniform1i(location, false);
    
    mat4 view = m_scene->sun()->viewMatrix();
    mat4 proj = m_scene->sun()->orthographicProjMatrix();
    mat4 toShadowMapSpace = proj * view;

    location = m_shader->getUniformLocation("ToShadowMapSpace");
    glUniformMatrix4fv(location, 1, GL_FALSE, value_ptr(toShadowMapSpace));
    
    CHECK_GL_ERRORS;
}

void Terrain::bindData()
{
    glBindBuffer( GL_ARRAY_BUFFER, m_vbo );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_ebo );
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureIDs[0]);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureIDs[1]);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_scene->shadowMapTexture());
    
    CHECK_GL_ERRORS;
}

void Terrain::drawElements()
{
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
    
    CHECK_GL_ERRORS;
}

void Terrain::releaseData()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);
    
    glBindBuffer( GL_ARRAY_BUFFER, 0 );
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    
    CHECK_GL_ERRORS;
}

