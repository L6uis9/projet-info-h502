#include "model.h"
#include "textureLoader.h"

#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <stdexcept>
#include <filesystem>

namespace fs = std::filesystem;

void Mesh::upload()
{
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(vertices.size() * sizeof(Vertex)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(indices.size() * sizeof(uint32_t)),
                 indices.data(), GL_STATIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, position));
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, normal));
    // texcoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                          (void*)offsetof(Vertex, texCoord));

    glBindVertexArray(0);
}

void Mesh::draw() const
{
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::free()
{
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1, &VBO);
    glDeleteVertexArrays(1, &VAO);
}

// MTL

static std::unordered_map<std::string, Material>
parseMTL(const std::string& mtlPath, const std::string& texDir)
{
    std::unordered_map<std::string, Material> mats;
    std::ifstream f(mtlPath);
    if (!f.is_open()) {
        std::cerr << "[Model] WARNING – cannot open MTL: " << mtlPath << "\n";
        return mats;
    }

    Material* cur = nullptr;
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "newmtl") {
            std::string name; ss >> name;
            mats[name] = Material{};
            mats[name].name = name;
            cur = &mats[name];
        } else if (cur && token == "Kd") {
            ss >> cur->Kd.r >> cur->Kd.g >> cur->Kd.b;
        } else if (cur && token == "map_Kd") {
            // The .mtl may reference "textures/Foo.jpg" – we strip to basename
            // and look in texDir so the project layout is self-contained.
            std::string rawPath; ss >> rawPath;
            // Handle Windows backslashes
            for (char& c : rawPath) if (c == '\\') c = '/';
            fs::path p(rawPath);
            std::string filename = p.filename().string();
            std::string fullPath = texDir + "/" + filename;
            cur->diffuseTexture = loadTexture(fullPath);
            cur->hasDiffuse     = true;
        }
    }
    return mats;
}

// OBJ

void Model::load(const std::string& objPath, const std::string& texDir)
{
    std::ifstream f(objPath);
    if (!f.is_open())
        throw std::runtime_error("[Model] Cannot open OBJ: " + objPath);

    // Raw attribute arrays
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;

    // MTL lookup
    std::unordered_map<std::string, Material> matLib;
    std::string objDir = fs::path(objPath).parent_path().string();
    if (objDir.empty()) objDir = ".";

    // Per-mesh accumulation
    struct FaceVertex { int p, t, n; };
    std::vector<FaceVertex> faceVerts;
    Material currentMat;

    auto flushMesh = [&]() {
        if (faceVerts.empty()) return;

        Mesh mesh;
        mesh.material = currentMat;

        std::unordered_map<std::string, uint32_t> cache;

        for (auto& fv : faceVerts) {
            std::string key = std::to_string(fv.p) + "/" +
                              std::to_string(fv.t) + "/" +
                              std::to_string(fv.n);

            auto it = cache.find(key);
            if (it != cache.end()) {
                mesh.indices.push_back(it->second);
            } else {
                Vertex v{};
                if (fv.p > 0 && fv.p <= (int)positions.size())
                    v.position = positions[fv.p - 1];
                if (fv.n > 0 && fv.n <= (int)normals.size())
                    v.normal = normals[fv.n - 1];
                if (fv.t > 0 && fv.t <= (int)texCoords.size())
                    v.texCoord = texCoords[fv.t - 1];

                uint32_t idx = (uint32_t)mesh.vertices.size();
                mesh.vertices.push_back(v);
                mesh.indices.push_back(idx);
                cache[key] = idx;
            }
        }

        mesh.upload();
        meshes.push_back(std::move(mesh));
        faceVerts.clear();
    };

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        // Strip carriage return
        if (!line.empty() && line.back() == '\r') line.pop_back();

        std::istringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "mtllib") {
            std::string mtlFile; ss >> mtlFile;
            std::string mtlPath = objDir + "/" + mtlFile;
            if (!fs::exists(mtlPath)) {
                mtlPath = objDir + "/spaceship.mtl";
            }
            matLib = parseMTL(mtlPath, texDir);
        } else if (token == "usemtl") {
            flushMesh();
            std::string matName; ss >> matName;
            auto it = matLib.find(matName);
            if (it != matLib.end()) currentMat = it->second;
            else { currentMat = Material{}; currentMat.name = matName; }
        } else if (token == "v") {
            glm::vec3 p; ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        } else if (token == "vn") {
            glm::vec3 n; ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (token == "vt") {
            glm::vec2 t; ss >> t.x >> t.y;
            texCoords.push_back(t);
        } else if (token == "f") {
            // Triangulate fan (works for convex faces)
            std::vector<FaceVertex> poly;
            std::string chunk;
            while (ss >> chunk) {
                FaceVertex fv{0,0,0};
                // formats: p, p/t, p//n, p/t/n
                int slashCount = (int)std::count(chunk.begin(), chunk.end(), '/');
                if (slashCount == 0) {
                    fv.p = std::stoi(chunk);
                } else {
                    std::replace(chunk.begin(), chunk.end(), '/', ' ');
                    std::istringstream cs(chunk);
                    cs >> fv.p;
                    if (slashCount == 2) { cs >> fv.t >> fv.n; }
                    else                 { cs >> fv.t; }
                }
                poly.push_back(fv);
            }
            for (int i = 1; i + 1 < (int)poly.size(); ++i) {
                faceVerts.push_back(poly[0]);
                faceVerts.push_back(poly[i]);
                faceVerts.push_back(poly[i + 1]);
            }
        }
    }

    flushMesh();
    std::cout << "[Model] Loaded " << meshes.size() << " meshes from " << objPath << "\n";
}

void Model::draw() const
{
    for (auto& m : meshes) m.draw();
}

void Model::free()
{
    for (auto& m : meshes) m.free();
    meshes.clear();
}