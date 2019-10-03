////////////////////////////////////////////////////////////////////////////////
// Filename: terrainclass.h
////////////////////////////////////////////////////////////////////////////////
#ifndef _TERRAINCLASS_H_
#define _TERRAINCLASS_H_


//////////////
// INCLUDES //
//////////////
#include "textureclass.h"
#include <d3d11.h>
#include <d3dx10math.h>
#include <stdio.h>
#include "perlin.h"
#include <queue>
#include <algorithm>
#include <time.h>

/////////////
// GLOBALS //
/////////////
const int TEXTURE_REPEAT = 16;

////////////////////////////////////////////////////////////////////////////////
// Class name: TerrainClass
////////////////////////////////////////////////////////////////////////////////
class TerrainClass
{
private:
	struct VertexType
	{
		D3DXVECTOR3 position;
		D3DXVECTOR2 texture;
	    D3DXVECTOR3 normal;
	};

	struct HeightMapType 
	{ 
		float x, y, z;
		float tu, tv;
		float nx, ny, nz;
	};

	struct VectorType 
	{ 
		float x, y, z;
	};

	struct dungeonCellData
	{
		float xTopRight, xBottomLeft, yTopRight, yBottomLeft;
	};

//	template<class dungeonCellData, class Container = std::index_sequence<dungeonCellData>> class queue;

public:
	TerrainClass();
	TerrainClass(const TerrainClass&);
	~TerrainClass();

	bool Initialize(ID3D11Device*, char*, WCHAR*, WCHAR*, WCHAR*);
	bool InitializeTerrain(ID3D11Device*, int terrainWidth, int terrainHeight, WCHAR*, WCHAR*, WCHAR*);
	void Shutdown();
	void Render(ID3D11DeviceContext*);
	bool GenerateHeightMap(ID3D11Device* device, bool keydown);
	int RandomHeightField();
	int SmoothVertex(ID3D11Device* device, bool keydown);
	int performPerlin(ID3D11Device* device, bool keydown);
	int spacePartitioning(ID3D11Device* device, bool keydown, int runs);
	void cellDivision(dungeonCellData currentCell);
	void roomGeneration();
	void roomHeight(int roomHeight);
	void corridorGeneration(int roomHeight);
	int GetIndexCount();

	ID3D11ShaderResourceView* GetGrassTexture();
	ID3D11ShaderResourceView* GetSlopeTexture();
	ID3D11ShaderResourceView* GetRockTexture();

private:
	bool LoadHeightMap(char*);
	void NormalizeHeightMap();
	bool CalculateNormals();
	void ShutdownHeightMap();

	void CalculateTextureCoordinates();
	bool LoadTextures(ID3D11Device*, WCHAR*, WCHAR*, WCHAR*);
	void ReleaseTextures();

	bool InitializeBuffers(ID3D11Device*);
	void ShutdownBuffers();
	void RenderBuffers(ID3D11DeviceContext*);
	
private:
	bool m_terrainGeneratedToggle, m_terrainSmoothToggle;
	int m_terrainWidth, m_terrainHeight;
	int m_vertexCount, m_indexCount;
	ID3D11Buffer *m_vertexBuffer, *m_indexBuffer;
	HeightMapType* m_heightMap;
	TextureClass *m_GrassTexture, *m_SlopeTexture, *m_RockTexture;

	dungeonCellData currentCell;
	dungeonCellData newCells[4];
	dungeonCellData heightCell;	std::deque <dungeonCellData> cellQueue;
	std::deque <dungeonCellData> roomQueue;
	std::deque <dungeonCellData> roomCopy;


	float midpointX; 
	float midpointY; 

	//perlin Perlin;
};

#endif