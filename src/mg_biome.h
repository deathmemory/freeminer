/*
biome.h
Copyright (C) 2010-2013 kwolekr, Ryan Kwolek <kwolekr@minetest.net>
*/

/*
This file is part of Freeminer.

Freeminer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Freeminer  is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Freeminer.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MG_BIOME_HEADER
#define MG_BIOME_HEADER

#include "objdef.h"
#include "nodedef.h"
#include "noise.h"

class Settings;
class BiomeManager;

////
//// Biome
////

#define BIOME_NONE ((u8)0)

#include "mapgen.h"

enum BiomeType
{
	BIOME_NORMAL,
	BIOME_LIQUID,
	BIOME_NETHER,
	BIOME_AETHER,
	BIOME_FLAT
};

class Biome : public ObjDef, public NodeResolver {
public:
	u32 flags;

	content_t c_top;
	content_t c_filler;
	content_t c_stone;
	content_t c_water_top;
	content_t c_water;
	content_t c_river_water;
	content_t c_dust;


	s16 depth_top;
	s16 depth_filler;
	s16 depth_water_top;

	s16 y_min;
	s16 y_max;
	float heat_point;
	float humidity_point;

	//freeminer:
	content_t c_ice;
	content_t c_top_cold;

	virtual void resolveNodeNames();
};


////
//// BiomeGen
////

enum BiomeGenType {
	BIOMEGEN_ORIGINAL,
};

struct BiomeParams {
	virtual void readParams(Settings *settings) = 0;
	virtual void writeParams(Settings *settings) const = 0;
	virtual ~BiomeParams() {}

	int seed;
};

class BiomeGen {
public:
	virtual ~BiomeGen() {}
	virtual BiomeGenType getType() const = 0;

	// Calculates the biome at the exact position provided.  This function can
	// be called at any time, but may be less efficient than the latter methods,
	// depending on implementation.
	virtual Biome *calcBiomeAtPoint(v3s16 pos) const = 0;

	// Computes any intermediate results needed for biome generation.  Must be
	// called before using any of: getBiomes, getBiomeAtPoint, or getBiomeAtIndex.
	// Calling this invalidates the previous results stored in biomemap.
	virtual void calcBiomeNoise(v3s16 pmin) = 0;

	// Gets all biomes in current chunk using each corresponding element of
	// heightmap as the y position, then stores the results by biome index in
	// biomemap (also returned)
	virtual u8 *getBiomes(s16 *heightmap) = 0;

	// Gets a single biome at the specified position, which must be contained
	// in the region formed by m_pmin and (m_pmin + m_csize - 1).
	virtual Biome *getBiomeAtPoint(v3s16 pos) const = 0;

	// Same as above, but uses a raw numeric index correlating to the (x,z) position.
	virtual Biome *getBiomeAtIndex(size_t index, s16 y) const = 0;

	// Result of calcBiomes bulk computation.
	u8 *biomemap;

protected:
	BiomeManager *m_bmgr;
	v3s16 m_pmin;
	v3s16 m_csize;
};


////
//// BiomeGen implementations
////

//
// Original biome algorithm (Whittaker's classification + surface height)
//

struct BiomeParamsOriginal : public BiomeParams {
	BiomeParamsOriginal() :
		np_heat(15, 30, v3f(750.0, 750.0, 750.0), 5349, 3, 0.5, 2.0),
		np_humidity(50, 50, v3f(750.0, 750.0, 750.0), 842, 3, 0.5, 2.0),
		np_heat_blend(0, 1.5, v3f(8.0, 8.0, 8.0), 13, 2, 1.0, 2.0),
		np_humidity_blend(0, 1.5, v3f(8.0, 8.0, 8.0), 90003, 2, 1.0, 2.0)
	{
	}

	virtual void readParams(Settings *settings);
	virtual void writeParams(Settings *settings) const;

	NoiseParams np_heat;
	NoiseParams np_humidity;
	NoiseParams np_heat_blend;
	NoiseParams np_humidity_blend;
};

class BiomeGenOriginal : public BiomeGen {
public:
	BiomeGenOriginal(BiomeManager *biomemgr,
		BiomeParamsOriginal *params, v3s16 chunksize);
	virtual ~BiomeGenOriginal();

	BiomeGenType getType() const { return BIOMEGEN_ORIGINAL; }

	Biome *calcBiomeAtPoint(v3s16 pos) const;
	void calcBiomeNoise(v3s16 pmin);

	u8 *getBiomes(s16 *heightmap);
	Biome *getBiomeAtPoint(v3s16 pos) const;
	Biome *getBiomeAtIndex(size_t index, s16 y) const;

	Biome *calcBiomeFromNoise(float heat, float humidity, s16 y) const;

	float *heatmap;
	float *humidmap;

private:
	BiomeParamsOriginal *m_params;

	Noise *noise_heat;
	Noise *noise_humidity;
	Noise *noise_heat_blend;
	Noise *noise_humidity_blend;
};


////
//// BiomeManager
////

class BiomeManager : public ObjDefManager {
public:
	BiomeManager(IGameDef *gamedef);
	virtual ~BiomeManager();

	const char *getObjectTitle() const
	{
		return "biome";
	}

	static Biome *create(BiomeType type)
	{
		return new Biome;
	}

	//freeminer:
	u32 year_days;
	s32 weather_heat_season;
	s32 weather_heat_width;
	s32 weather_heat_daily;
	s32 weather_heat_height;
	s32 weather_humidity_season;
	s32 weather_humidity_width;
	s32 weather_humidity_daily;
	s32 weather_humidity_days;
	s32 weather_hot_core;

	MapgenParams * mapgen_params;
	s16 calcBlockHeat(v3s16 p, uint64_t seed, float timeofday, float totaltime, bool use_weather = 1);
	s16 calcBlockHumidity(v3s16 p, uint64_t seed, float timeofday, float totaltime, bool use_weather = 1);
	//====

	BiomeGen *createBiomeGen(BiomeGenType type, BiomeParams *params, v3s16 chunksize)
	{
		switch (type) {
		case BIOMEGEN_ORIGINAL:
			return new BiomeGenOriginal(this,
				(BiomeParamsOriginal *)params, chunksize);
		default:
			return NULL;
		}
	}

	static BiomeParams *createBiomeParams(BiomeGenType type)
	{
		switch (type) {
		case BIOMEGEN_ORIGINAL:
			return new BiomeParamsOriginal;
		default:
			return NULL;
		}
	}

	virtual void clear();

	// Looks for pos in the biome cache, and if non-existent, looks up by noise
	u8 getBiomeAtPoint(v3s16 pos);

private:
	IGameDef *m_gamedef;

};


#endif
