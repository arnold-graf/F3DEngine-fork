#pragma once

#include <nlohmann/json.hpp>
#include "../globals.h"

using json = nlohmann::json;

namespace vecSys {
    inline void to_json(json& j, const vec2& val) {
        j = json{{"x", val.x}, {"y", val.y}};
    }

    inline void from_json(const json& j, vec2& val) {
        j.at("x").get_to(val.x);
        j.at("y").get_to(val.y);
        // using namespace std;
        // cout << format("vec2: x {} y {}", val.x, val.y) << endl;
    }

    inline void to_json(json& j, const vec3& val) {
        j = json{{"x", val.x}, {"y", val.y}, {"z", val.z}};
    }

    inline void from_json(const json& j, vec3& val) {
        j.at("x").get_to(val.x);
        j.at("y").get_to(val.y);
        j.at("z").get_to(val.z);
        // using namespace std;
        // cout << format("vec3: x {} y {} z {}", val.x, val.y, val.z) << endl;
    }

    inline void to_json(json& j, const line2& val) {
        j = json{{"pnt1", val.pnt1}, {"pnt2", val.pnt2}};
    }

    inline void from_json(const json& j, line2& val) {
        j.at("pnt1").get_to(val.pnt1);
        j.at("pnt2").get_to(val.pnt2);
        // using namespace std;
        // cout << format("Line2: pnt1 {},{} pnt2 {},{}", val.pnt1.x, val.pnt1.y , val.pnt2.x, val.pnt1.y) << endl;
    }

    inline void to_json(json& j, const shape2& val) {
        j = json{{"points", val.points}};
    }

    inline void from_json(const json& j, shape2& val) {
        j.at("points").get_to(val.points);
        // using namespace std;
        // cout << "Shape2: " << endl;
        // for (const vec2& v: val.points){
        //     cout << format("vec2: pnt1 {} pnt2 {}", v.x, v.y) << endl;
        // }
    }
};

// --- Sector ---
inline void to_json(json& j, const Sector::Wall& val) {
    j = {
        {"isBarrier", val.isBarrier},
        {"isVisibleWall", val.isVisibleWall},
        {"wallHeight", val.wallHeight},
        {"textureFile", val.textureFile}
    };
}

inline void from_json(const json& j, Sector::Wall& val){
    j.at("isBarrier").get_to(val.isBarrier);
    j.at("isVisibleWall").get_to(val.isVisibleWall);
    j.at("wallHeight").get_to(val.wallHeight);
    j.at("textureFile").get_to(val.textureFile);
    // std::cout << std::format("sector::wall texture: {}", val.textureFile) << std::endl;
}

inline void to_json(json& j, const Sector& val) {
    j = {
        {"outline", val.outline},
        {"rotation", val.rotation},
        {"baseHeight", val.baseHeight},
        {"floatingHeight", val.floatingHeight},
        {"walls", val.walls},
        {"floorTextureFile", val.floorTextureFile},
        {"bottomTextureFile", val.bottomTextureFile},
        {"baseWallTextureFile", val.baseWallTextureFile}
    };
}

inline void from_json(const json& j, Sector& val) {
    j.at("outline").get_to(val.outline);
    j.at("rotation").get_to(val.rotation);
    j.at("baseHeight").get_to(val.baseHeight);
    j.at("floatingHeight").get_to(val.floatingHeight);
    j.at("walls").get_to(val.walls);
    j.at("floorTextureFile").get_to(val.floorTextureFile);
    j.at("bottomTextureFile").get_to(val.bottomTextureFile);
    j.at("baseWallTextureFile").get_to(val.baseWallTextureFile);
    // std::cout << std::format("sector floorTextureFile: {}", val.floorTextureFile) << std::endl;
    // std::cout << std::format("sector bottomTextureFile: {}", val.bottomTextureFile) << std::endl;
    // std::cout << std::format("sector baseWallTextureFile: {}", val.baseWallTextureFile) << std::endl;
}

// --- Billboard ---

inline void to_json(json& j, const Billboard& val) {
    j = {
        {"baseLine", val.base},
        {"verticalOffset", val.verticalOffset},
        {"Height", val.height},
        {"isVisible", val.visible},
        {"TextureFile", val.textureFile}
    };
}

inline void from_json(const json& j, Billboard& val) {
    j.at("baseLine").get_to(val.base);
    j.at("verticalOffset").get_to(val.verticalOffset);
    j.at("Height").get_to(val.height);
    j.at("isVisible").get_to(val.visible);
    j.at("TextureFile").get_to(val.textureFile);
    // std::cout << std::format("billboard texture: {}", val.textureFile) << std::endl;
}

// --- Light ---

inline void to_json(json& j, const Light& val) {
    j = {
        {"color", json::array({val.color.r, val.color.g, val.color.b, val.color.a})},
        {"position", val.position},
        {"zElevation", val.zElevation},
        {"radiance", val.radiance},
        {"pitch", val.pitch},
        {"yaw", val.yaw},
        {"halfFOVH", val.halfFOVH},
        {"halfFOVV", val.halfFOVV},
        {"isON", val.isON},
        {"flicker", val.flicker}
    };
}

inline void from_json(const json& j, Light& val) {
    auto color = j.at("color");
    val.color.r = color.at(0).get<double>();
    val.color.g = color.at(1).get<double>();
    val.color.b = color.at(2).get<double>();
    val.color.a = color.size() > 3 ? color.at(3).get<double>() : 1.0;
    j.at("position").get_to(val.position);
    j.at("zElevation").get_to(val.zElevation);
    j.at("radiance").get_to(val.radiance);
    if (j.contains("pitch")) j.at("pitch").get_to(val.pitch);
    if (j.contains("yaw")) j.at("yaw").get_to(val.yaw);
    if (j.contains("halfFOVH")) j.at("halfFOVH").get_to(val.halfFOVH);
    if (j.contains("halfFOVV")) j.at("halfFOVV").get_to(val.halfFOVV);
    if (j.contains("isON")) j.at("isON").get_to(val.isON);
    if (j.contains("flicker")) j.at("flicker").get_to(val.flicker);
    val.cuttoffMinH = val.yaw - val.halfFOVH + pi;
    val.cuttoffMaxH = val.yaw + val.halfFOVH + pi;
    val.cuttoffminV = val.pitch - val.halfFOVV + pi;
    val.cuttoffMaxV = val.pitch + val.halfFOVV + pi;
}

// --- Terrain ---

inline void to_json(json& j, const Mountain& val) {
    j = {
        {"center", vec2{val.centerX, val.centerY}},
        {"radius", val.radius},
        {"peakHeight", val.peakHeight},
        {"detailScale", val.detailScale},
        {"seed", val.seed}
    };
}

inline void from_json(const json& j, Mountain& val) {
    vec2 center {};
    j.at("center").get_to(center);
    val.centerX = center.x;
    val.centerY = center.y;
    j.at("radius").get_to(val.radius);
    j.at("peakHeight").get_to(val.peakHeight);
    if (j.contains("detailScale")) j.at("detailScale").get_to(val.detailScale);
    if (j.contains("seed")) j.at("seed").get_to(val.seed);
}

inline void to_json(json& j, const TerrainSettings& val) {
    j = {
        {"enabled", val.enabled},
        {"seed", val.seed},
        {"noiseScale", val.noiseScale},
        {"heightAmplitude", val.heightAmplitude},
        {"baseHeight", val.baseHeight},
        {"stepSize", val.stepSize},
        {"fbmOctaves", val.fbmOctaves},
        {"fbmPersistence", val.fbmPersistence},
        {"fbmLacunarity", val.fbmLacunarity},
        {"textureFile", val.textureFile},
        {"groundSectorIndex", val.groundSectorIndex},
        {"mountains", val.mountains}
    };
}

inline void from_json(const json& j, TerrainSettings& val) {
    if (j.contains("enabled")) j.at("enabled").get_to(val.enabled);
    if (j.contains("seed")) j.at("seed").get_to(val.seed);
    if (j.contains("noiseScale")) j.at("noiseScale").get_to(val.noiseScale);
    if (j.contains("heightAmplitude")) j.at("heightAmplitude").get_to(val.heightAmplitude);
    if (j.contains("baseHeight")) j.at("baseHeight").get_to(val.baseHeight);
    if (j.contains("stepSize")) j.at("stepSize").get_to(val.stepSize);
    if (j.contains("fbmOctaves")) j.at("fbmOctaves").get_to(val.fbmOctaves);
    if (j.contains("fbmPersistence")) j.at("fbmPersistence").get_to(val.fbmPersistence);
    if (j.contains("fbmLacunarity")) j.at("fbmLacunarity").get_to(val.fbmLacunarity);
    if (j.contains("textureFile")) j.at("textureFile").get_to(val.textureFile);
    if (j.contains("groundSectorIndex")) j.at("groundSectorIndex").get_to(val.groundSectorIndex);
    if (j.contains("mountains")) j.at("mountains").get_to(val.mountains);
}

// --- Level ---

inline void to_json(json& j, const Level& val) {
    j = {
        {"sectorList", val.sectorList},
        {"billboardList", val.billboardList},
        {"lightList", val.lightList},
        {"terrain", val.terrain}
    };
}

inline void from_json(const json& j, Level& val) {
    j.at("sectorList").get_to(val.sectorList);
    j.at("billboardList").get_to(val.billboardList);
    if (j.contains("lightList")) j.at("lightList").get_to(val.lightList);
    if (j.contains("terrain")) j.at("terrain").get_to(val.terrain);
}