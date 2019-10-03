////////////////////////////////////////////////////////////////////////////////
// Filename: terrainclass.cpp
////////////////////////////////////////////////////////////////////////////////
#include "terrainclass.h"
#include <cmath>


TerrainClass::TerrainClass()
{
	m_vertexBuffer = 0;
	m_indexBuffer = 0;
	m_heightMap = 0;
	m_terrainGeneratedToggle = false;
	m_terrainSmoothToggle = false;

	m_GrassTexture = 0;
	m_SlopeTexture = 0;
	m_RockTexture = 0;

}

TerrainClass::TerrainClass(const TerrainClass& other)
{
}

TerrainClass::~TerrainClass()
{
}

bool TerrainClass::InitializeTerrain(ID3D11Device* device, int terrainWidth, int terrainHeight, WCHAR* grassTextureFilename, WCHAR* slopeTextureFilename, WCHAR* rockTextureFilename)
{
	int index;
	float height = 0.0;
	bool result;

	// Save the dimensions of the terrain.
	m_terrainWidth = terrainWidth;
	m_terrainHeight = terrainHeight;

	// Create the structure to hold the terrain data.
	m_heightMap = new HeightMapType[m_terrainWidth * m_terrainHeight];
	if(!m_heightMap)
	{
		return false;
	}

	// Initialise the data in the height map (flat).
	for(int j=0; j<m_terrainHeight; j++)
	{
		for(int i=0; i<m_terrainWidth; i++)
		{			
			index = (m_terrainWidth * j) + i;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = (float)height;
			m_heightMap[index].z = (float)j;

		}
	}

	//even though we are generating a flat terrain, we still need to normalise it. 
	// Calculate the normals for the terrain data.
	result = CalculateNormals();
	if(!result)
	{
		return false;
	}



	// Calculate the texture coordinates.
	CalculateTextureCoordinates();
	// Load the texture.
	result = LoadTextures(device, grassTextureFilename, slopeTextureFilename, rockTextureFilename);
	if(!result)
	{
		return false;
	}

	// Initialize the vertex and index buffer that hold the geometry for the terrain.
	result = InitializeBuffers(device);
	if(!result)
	{
		return false;
	}

	return true;
}

bool TerrainClass::Initialize(ID3D11Device* device, char* heightMapFilename, WCHAR* grassTextureFilename, WCHAR* slopeTextureFilename, WCHAR* rockTextureFilename)
{
	bool result;

	// Load in the height map for the terrain.
	result = LoadHeightMap(heightMapFilename);
	if(!result)
	{
		return false;
	}

	// Normalize the height of the height map.
	NormalizeHeightMap();

	// Calculate the normals for the terrain data.
	result = CalculateNormals();
	if(!result)
	{
		return false;
	}

	// Calculate the texture coordinates.
	CalculateTextureCoordinates();
	// Load the texture.
	result = LoadTextures(device, grassTextureFilename, slopeTextureFilename, rockTextureFilename);
	if (!result)
	{
		return false;
	}


	// Initialize the vertex and index buffer that hold the geometry for the terrain.
	result = InitializeBuffers(device);
	if(!result)
	{
		return false;
	}

	return true;
}

void TerrainClass::Shutdown()
{
	// Release the texture.
	ReleaseTextures();

	// Release the vertex and index buffer.
	ShutdownBuffers();

	// Release the height map data.
	ShutdownHeightMap();

	

	return;
}

void TerrainClass::Render(ID3D11DeviceContext* deviceContext)
{
	// Put the vertex and index buffers on the graphics pipeline to prepare them for drawing.
	RenderBuffers(deviceContext);

	return;
}

int TerrainClass::GetIndexCount()
{
	return m_indexCount;
}

ID3D11ShaderResourceView* TerrainClass::GetGrassTexture()
{
	return m_GrassTexture->GetTexture();
}

ID3D11ShaderResourceView* TerrainClass::GetSlopeTexture()
{
	return m_SlopeTexture->GetTexture();
}

ID3D11ShaderResourceView* TerrainClass::GetRockTexture()
{
	return m_RockTexture->GetTexture();
}

bool TerrainClass::GenerateHeightMap(ID3D11Device* device, bool keydown)
{

	bool result;
	//the toggle is just a bool that I use to make sure this is only called ONCE when you press a key
	//until you release the key and start again. We dont want to be generating the terrain 500
	//times per second. 
	if(keydown&&(!m_terrainGeneratedToggle))
	{
		int index;
		float height = 0.0;
		

		//loop through the terrain and set the hieghts how we want. This is where we generate the terrain
		//in this case I will run a sin-wave through the terrain in one axis.

 		for(int j=0; j<m_terrainHeight; j++)
		{
			for(int i=0; i<m_terrainWidth; i++)
			{			
				index = (m_terrainHeight * j) + i;

				m_heightMap[index].x = (float)i;
				m_heightMap[index].y = (float)(RandomHeightField()); //magic numbers ahoy, just to ramp up the height of the sin function so its visible.
				m_heightMap[index].z = (float)j;
			}
		}

		result = CalculateNormals();
		if(!result)
		{
			return false;
		}

		// Initialize the vertex and index buffer that hold the geometry for the terrain.
		result = InitializeBuffers(device);
		if(!result)
		{
			return false;
		}

		m_terrainGeneratedToggle = true;
	}
	/*if (!keydown && (m_terrainGeneratedToggle))
	{
		m_terrainGeneratedToggle = false;
	}*/

	return true;
}

int TerrainClass::RandomHeightField()
{
	int randomHeight = 0;

	//srand(NULL);
	
	randomHeight = rand() % 12 + 1;

	return randomHeight;
}

int TerrainClass::SmoothVertex(ID3D11Device* device, bool keydown)
{
	bool result;
	//the toggle is just a bool that I use to make sure this is only called ONCE when you press a key
	//until you release the key and start again. We dont want to be generating the terrain 500
	//times per second. 
	if (keydown && (!m_terrainSmoothToggle))
	{
		int index;
		float average;
		float height = 0.0;


		//loop through the terrain and set the hieghts how we want. This is where we generate the terrain
		//in this case I will run a sin-wave through the terrain in one axis.

		for (int j = 0; j < m_terrainHeight; j++)
		{
			for (int i = 0; i < m_terrainWidth; i++)
			{
				average = 0.0f;
				//// Grab the average of the surrounding points
				// (-1, -1)
				index = (m_terrainHeight * (j - 1) + (i - 1));
				if ((index < m_terrainHeight * m_terrainWidth) && (index > 0))
				{
					average += m_heightMap[index].y;
				}

				// (0, -1)
				index = (m_terrainHeight * (j - 1) + i);
				if ((index < m_terrainHeight * m_terrainWidth) && (index > 0))
				{
					average += m_heightMap[index].y;
				}
				// (1, -1)
				index = (m_terrainHeight * (j - 1) + (i + 1));
				if ((index < m_terrainHeight * m_terrainWidth) && (index > 0))
				{
					average += m_heightMap[index].y;
				}
				// (-1, 0)
				index = (m_terrainHeight * j + (i - 1));
				if ((index < m_terrainHeight * m_terrainWidth) && (index > 0))
				{
					average += m_heightMap[index].y;
				}
				// (0, 0)
				index = (m_terrainHeight * j) + i;
				if ((index < m_terrainHeight * m_terrainWidth) && (index > 0))
				{
					average += m_heightMap[index].y;
				}
				// (1, 0)
				index = (m_terrainHeight * j + (i + 1));
				if ((index < m_terrainHeight * m_terrainWidth) && (index > 0))
				{
					average += m_heightMap[index].y;
				}
				// (-1, 1)
				index = (m_terrainHeight * (j + 1) + (i - 1));
				if ((index < m_terrainHeight * m_terrainWidth) && (index > 0))
				{
					average += m_heightMap[index].y;
				}
				// (0, 1)
				index = (m_terrainHeight * (j + 1) + i);
				if ((index < m_terrainHeight * m_terrainWidth) && (index > 0))
				{
					average += m_heightMap[index].y;
				}
				// (1, 1)
				index = (m_terrainHeight * (j + 1) + (i + 1));
				if ((index < m_terrainHeight * m_terrainWidth) && (index > 0))
				{
					average += m_heightMap[index].y;
				}

				average = average / 9;

				index = (m_terrainHeight * j) + i;

				m_heightMap[index].x = (float)i;
				m_heightMap[index].y = average;
				m_heightMap[index].z = (float)j;
			}
		}

		result = CalculateNormals();
		if (!result)
		{
			return false;
		}

		// Initialize the vertex and index buffer that hold the geometry for the terrain.
		result = InitializeBuffers(device);
		if (!result)
		{
			return false;
		}

		m_terrainSmoothToggle = false;
	}

//else
//{
//	m_terrainSmoothToggle = false;
//}

	return true;
}

int TerrainClass::performPerlin(ID3D11Device * device, bool keydown)
{
	bool result;

	if (keydown && (!m_terrainGeneratedToggle))
	{
		int index;
		float height = 1.0;

		perlin Perlin(58);

		for (int performPerlin = 0; performPerlin < 4; performPerlin++)
		{
			for (int j = 0; j<m_terrainHeight; j++)
			{
				for (int i = 0; i<m_terrainWidth; i++)
				{
					index = (m_terrainHeight * j) + i;

					m_heightMap[index].x = (float)i;
					m_heightMap[index].y = m_heightMap[index].y + 0.1* (float)((Perlin.noise(i, j, 1.1))*m_heightMap[index].y);
					m_heightMap[index].z = (float)j;
				}
			}
		}
		

		result = CalculateNormals();
		if (!result)
		{
			return false;
		}

		result = InitializeBuffers(device);
		if (!result)
		{
			return false;
		}

		m_terrainGeneratedToggle = true;
	}
	if (!keydown && (m_terrainGeneratedToggle))
	{
		m_terrainGeneratedToggle = false;
	}

	return true;
}

void TerrainClass::corridorGeneration(int roomHeight)
{
	dungeonCellData roomConnections[2];
	dungeonCellData roomsToConnect[2];

	int index = 0;
	int roomCopySize = roomCopy.size();


	for (int i = 0; i < roomCopySize - 1; i++)
	{
		// The two rooms to connect are the first two in the queue
		roomsToConnect[0] = roomCopy.front();
		roomCopy.pop_front();
		roomsToConnect[1] = roomCopy.front();

		// Connects the two cells by two rectangles, one in X and one in Y
		// to the Top Left
		if ((roomsToConnect[0].xBottomLeft > roomsToConnect[1].xTopRight) && (roomsToConnect[0].yTopRight < roomsToConnect[1].yBottomLeft))
		{
			roomConnections[0].xBottomLeft = roomsToConnect[0].xBottomLeft;
			roomConnections[0].yBottomLeft = roomsToConnect[0].yTopRight;

			roomConnections[0].xTopRight = roomsToConnect[0].xBottomLeft + 3;
			roomConnections[0].yTopRight = roomsToConnect[1].yBottomLeft + 3;

			roomConnections[1].xBottomLeft = roomsToConnect[1].xTopRight;
			roomConnections[1].yBottomLeft = roomsToConnect[1].yBottomLeft;

			roomConnections[1].xTopRight = roomConnections[0].xTopRight;
			roomConnections[1].yTopRight = roomsToConnect[1].yBottomLeft + 3;

			for (int y = roomConnections[0].yBottomLeft; y < roomConnections[0].yTopRight; y++)
			{
				for (int x = roomConnections[0].xBottomLeft; x < roomConnections[0].xTopRight; x++)
				{
					index = (y * m_terrainWidth) + (x);
					if (index < (m_terrainHeight * m_terrainWidth) + 1)
					{
						m_heightMap[index].y = -roomHeight;
					}
				}
			}
			for (int y = roomConnections[1].yBottomLeft; y < roomConnections[0].yTopRight; y++)
			{
				for (int x = roomConnections[1].xBottomLeft; x < roomConnections[1].xTopRight; x++)
				{
					index = (y * m_terrainWidth) + (x);
					if (index < (m_terrainHeight * m_terrainWidth) + 1)
					{
						m_heightMap[index].y = -roomHeight;
					}
				}
			}
		}

		// to the Top Right
		if ((roomsToConnect[0].xTopRight < roomsToConnect[1].xBottomLeft) && (roomsToConnect[0].yTopRight < roomsToConnect[1].yBottomLeft))
		{
			roomConnections[0].xBottomLeft = roomsToConnect[0].xTopRight - 3;
			roomConnections[0].yBottomLeft = roomsToConnect[0].yTopRight;

			roomConnections[0].xTopRight = roomsToConnect[0].xTopRight;
			roomConnections[0].yTopRight = roomsToConnect[1].yBottomLeft + 3;

			roomConnections[1].xBottomLeft = roomsToConnect[0].xTopRight - 3;
			roomConnections[1].yBottomLeft = roomsToConnect[1].yBottomLeft;

			roomConnections[1].xTopRight = roomConnections[1].xBottomLeft;
			roomConnections[1].yTopRight = roomsToConnect[1].yBottomLeft + 3;

			for (int y = roomConnections[0].yBottomLeft; y < roomConnections[0].yTopRight; y++)
			{
				for (int x = roomConnections[0].xBottomLeft; x < roomConnections[0].xTopRight; x++)
				{
					index = (y * m_terrainWidth) + (x);
					if (index < (m_terrainHeight * m_terrainWidth) + 1)
					{
						m_heightMap[index].y = -roomHeight;
					}
				}
			}
			for (int y = roomConnections[1].yBottomLeft; y < roomConnections[0].yTopRight; y++)
			{
				for (int x = roomConnections[1].xBottomLeft; x < roomConnections[1].xTopRight; x++)
				{
					index = (y * m_terrainWidth) + (x);
					if (index < (m_terrainHeight * m_terrainWidth) + 1)
					{
						m_heightMap[index].y = -roomHeight;
					}
				}
			}
		}

		// to the Bottom Left
		if ((roomsToConnect[0].xBottomLeft > roomsToConnect[1].xTopRight) && (roomsToConnect[0].yBottomLeft < roomsToConnect[1].yTopRight))
		{
			roomConnections[0].xBottomLeft = roomsToConnect[1].xTopRight - 3;
			roomConnections[0].yBottomLeft = roomsToConnect[0].yBottomLeft;

			roomConnections[0].xTopRight = roomsToConnect[0].xBottomLeft;
			roomConnections[0].yTopRight = roomsToConnect[0].yBottomLeft + 3;

			roomConnections[1].xBottomLeft = roomsToConnect[1].xTopRight - 3;
			roomConnections[1].yBottomLeft = roomsToConnect[1].yTopRight;

			roomConnections[1].xTopRight = roomConnections[1].xTopRight;
			roomConnections[1].yTopRight = roomsToConnect[0].yBottomLeft + 3;

			for (int y = roomConnections[0].yBottomLeft; y < roomConnections[0].yTopRight; y++)
			{
				for (int x = roomConnections[0].xBottomLeft; x < roomConnections[0].xTopRight; x++)
				{
					index = (y * m_terrainWidth) + (x);
					if (index < (m_terrainHeight * m_terrainWidth) + 1)
					{
						m_heightMap[index].y = -roomHeight;
					}
				}
			}
			for (int y = roomConnections[1].yBottomLeft; y < roomConnections[0].yTopRight; y++)
			{
				for (int x = roomConnections[1].xBottomLeft; x < roomConnections[1].xTopRight; x++)
				{
					index = (y * m_terrainWidth) + (x);
					if (index < (m_terrainHeight * m_terrainWidth) + 1)
					{
						m_heightMap[index].y = -roomHeight;
					}
				}
			}
		}

		// to the Bottom Right
		if ((roomsToConnect[0].xTopRight < roomsToConnect[1].xBottomLeft) && (roomsToConnect[0].yBottomLeft > roomsToConnect[1].yTopRight))
		{
			roomConnections[0].xBottomLeft = roomsToConnect[0].xTopRight;
			roomConnections[0].yBottomLeft = roomsToConnect[0].yBottomLeft;

			roomConnections[0].xTopRight = roomsToConnect[1].xBottomLeft + 3;
			roomConnections[0].yTopRight = roomsToConnect[0].yBottomLeft + 3;

			roomConnections[1].xBottomLeft = roomsToConnect[1].xBottomLeft;
			roomConnections[1].yBottomLeft = roomsToConnect[1].yTopRight;

			roomConnections[1].xTopRight = roomConnections[1].xBottomLeft + 3;
			roomConnections[1].yTopRight = roomsToConnect[0].yBottomLeft + 3;

			for (int y = roomConnections[0].yBottomLeft; y < roomConnections[0].yTopRight; y++)
			{
				for (int x = roomConnections[0].xBottomLeft; x < roomConnections[0].xTopRight; x++)
				{
					index = (y * m_terrainWidth) + (x);
					if (index < (m_terrainHeight * m_terrainWidth) + 1)
					{
						m_heightMap[index].y = -roomHeight;
					}
				}
			}
			for (int y = roomConnections[1].yBottomLeft; y < roomConnections[0].yTopRight; y++)
			{
				for (int x = roomConnections[1].xBottomLeft; x < roomConnections[1].xTopRight; x++)
				{
					index = (y * m_terrainWidth) + (x);
					if (index < (m_terrainHeight * m_terrainWidth) + 1)
					{
						m_heightMap[index].y = -roomHeight;
					}
				}
			}
		}
	}

	return;
}

void TerrainClass::roomHeight(int roomHeight)
{
	dungeonCellData room;
	int index = 0;
	int indexB = 0;
	std::deque<int>::size_type roomQueueSize = roomQueue.size();

	// Resets the rest of the map to have height 0 so that rooms dont stack on top of each other over time
	for (int xLoop = 0; xLoop < m_terrainWidth; xLoop++)
	{
		for (int yLoop = 0; yLoop < m_terrainHeight; yLoop++)
		{
			indexB = (yLoop * m_terrainWidth) + (xLoop);
			m_heightMap[indexB].y = 0;
		}
	}

	// Loops through room queue, and brings whole height down to 5
	for (int roomNum = 0; roomNum < roomQueue.size();)
	{
		room = roomQueue.front();

		for (int y = room.yBottomLeft; y < room.yTopRight; y++)
		{
			for (int x = room.xBottomLeft; x < room.xTopRight; x++)
			{
				index = (y * m_terrainWidth) + (x);
				if (index < (m_terrainHeight * m_terrainWidth) + 1)
				{
					m_heightMap[index].y = -roomHeight;
				}
			}
		}
		roomQueue.pop_front();
	}
}

void TerrainClass::roomGeneration()
{
	int cellMid[2];
	dungeonCellData newRoom;
	
	// For each cell in the queue, randomly generate a size within the outer bounds
	for (int cellNum = 0; cellNum < cellQueue.size(); )
		{
			heightCell = cellQueue.front();

			cellMid[0] = (heightCell.xBottomLeft + heightCell.xTopRight) / 2;
			cellMid[1] = (heightCell.yBottomLeft + heightCell.yTopRight) / 2;

			newRoom.xBottomLeft = (rand() % cellMid[0] + heightCell.xBottomLeft);
			newRoom.yBottomLeft = (rand() % cellMid[1] + heightCell.yBottomLeft);

			newRoom.xTopRight = (rand() % (int)heightCell.xTopRight + cellMid[0]);
			newRoom.yTopRight = (rand() % (int)heightCell.yTopRight + cellMid[1]);

			roomQueue.push_back(newRoom);

			cellQueue.pop_front();
		}

	// Copies queue for use in the height/corridor generation functions
	roomCopy = roomQueue;

	roomHeight(8);
	corridorGeneration(8);

	return;
}

void TerrainClass::cellDivision(dungeonCellData currentCell)
{

	// Sets the initial parent cell as the first to be split
	currentCell = cellQueue.front();

	midpointX = (currentCell.xTopRight + currentCell.xBottomLeft) / 2.0f;
	midpointY = (currentCell.yTopRight + currentCell.yBottomLeft) / 2.0f;

	// Sets up the lowest corner, e.g. start of cell
	for (int i = 0; i < 4; i++)
	{
		newCells[i].xTopRight = currentCell.xBottomLeft;
		newCells[i].yTopRight = currentCell.yBottomLeft;
		newCells[i].xBottomLeft = currentCell.xBottomLeft;
		newCells[i].yBottomLeft = currentCell.yBottomLeft;
	}

	// ===============================================================
	for (float y = currentCell.yBottomLeft; y < currentCell.yTopRight; y++)
	{
		for (float x = currentCell.xBottomLeft; x < currentCell.xTopRight; x++)
		{
			// LEFT HAND CELLS
			if (x < midpointX)
			{
				if (y < midpointY)
				{
					// Bottom left cell
					if (x >= newCells[0].xTopRight)
					{
						newCells[0].xTopRight = x;
					}

					if (y >= newCells[0].yTopRight)
					{
						newCells[0].yTopRight = y;
					}
					// queue.add cell[index]
					newCells[0].xBottomLeft = currentCell.xBottomLeft;
					newCells[0].yBottomLeft = currentCell.yBottomLeft;
				}
				else if (y >= midpointY)
				{
					// Top left cell
					if (x >= newCells[1].xTopRight)
					{

						newCells[1].xTopRight = x;
					}

					if (y >= newCells[1].yTopRight)
					{
						newCells[1].yTopRight = y;
					}
					//queue.add cell[index]
					newCells[1].xBottomLeft = currentCell.xBottomLeft;
					newCells[1].yBottomLeft = midpointY;
				}

			}


			// RIGHT HAND CELLS
			else if (x >= midpointX)
			{
				if (y < midpointY)
				{
					// Bottom right cell
					if (x >= newCells[2].xTopRight)
					{
						newCells[2].xTopRight = x;
					}

					if (y >= newCells[2].yTopRight)
					{
						newCells[2].yTopRight = y;
					}
					//queue.add cell[index]
					newCells[2].xBottomLeft = midpointX;
					newCells[2].yBottomLeft = currentCell.yBottomLeft;
				}
				else if (y >= midpointY)
				{
					// Top right cell
					if (x >= newCells[3].xTopRight)
					{
						newCells[3].xTopRight = x;
					}

					if (y >= newCells[3].yTopRight)
					{
						newCells[3].yTopRight = y;
					}
					//queue.add cell[index]
					newCells[3].xBottomLeft = midpointX;
					newCells[3].yBottomLeft = midpointY;
				}
			}
		}
	}


	// Pushes the new children cells to the queue of total cells
	for (int push = 0; push < 4; push++)
	{
		cellQueue.push_back(newCells[push]);
	}
	//================================================================
	// Removes the parent cell from the queue
	cellQueue.pop_front();
}

int TerrainClass::spacePartitioning(ID3D11Device* device, bool keydown, int runs)
{
	if (keydown && (!m_terrainGeneratedToggle))
	{
		srand(time(NULL));
		m_terrainGeneratedToggle = true;
		bool result;

		// First cell is full terrain, current cell iterates through created cell list to make more
		currentCell.xBottomLeft = 0.0f;
		currentCell.yBottomLeft = 0.0f;
		currentCell.xTopRight = m_terrainWidth;
		currentCell.yTopRight = m_terrainHeight;

		// Cell for adjusting height values after loop
		heightCell.xBottomLeft = 0.0f;
		heightCell.yBottomLeft = 0.0f;
		heightCell.xTopRight = 0.0f;
		heightCell.yTopRight = 0.0f;
	
		cellQueue.push_front(currentCell);

		// Storage for new cells to be created from old
	
		for (int newCell = 0; newCell < 4; newCell++)
		{
			newCells[newCell].xBottomLeft = 0.0f;
			newCells[newCell].yBottomLeft = 0.0f;
			newCells[newCell].xTopRight = 0.0f;
			newCells[newCell].yTopRight = 0.0f;
		}
	
		// Calls the cell division function (quad tree) for a random amount of times between 10 and a random number (20-40)
		for (int divisionPass = 0; divisionPass < (rand() % (rand() % 50 + 40) + 20); divisionPass++)
		{
			cellDivision(cellQueue.front());
		}

		// Calls the room generation function to split up the new cells into smaller rooms within each
		roomGeneration();

		result = CalculateNormals();
		if (!result)
		{
			return false;
		}

		// Initialize the vertex and index buffer that hold the geometry for the terrain.
		result = InitializeBuffers(device);
		if (!result)
		{
			return false;
		}

		m_terrainGeneratedToggle = false;

	}
	

	return true;
}

bool TerrainClass::LoadHeightMap(char* filename)
{
	FILE* filePtr;
	int error;
	unsigned int count;
	BITMAPFILEHEADER bitmapFileHeader;
	BITMAPINFOHEADER bitmapInfoHeader;
	int imageSize, i, j, k, index;
	unsigned char* bitmapImage;
	unsigned char height;


	// Open the height map file in binary.
	error = fopen_s(&filePtr, filename, "rb");
	if(error != 0)
	{
		return false;
	}

	// Read in the file header.
	count = fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);
	if(count != 1)
	{
		return false;
	}

	// Read in the bitmap info header.
	count = fread(&bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);
	if(count != 1)
	{
		return false;
	}

	// Save the dimensions of the terrain.
	m_terrainWidth = bitmapInfoHeader.biWidth;
	m_terrainHeight = bitmapInfoHeader.biHeight;

	// Calculate the size of the bitmap image data.
	imageSize = m_terrainWidth * m_terrainHeight * 3;

	// Allocate memory for the bitmap image data.
	bitmapImage = new unsigned char[imageSize];
	if(!bitmapImage)
	{
		return false;
	}

	// Move to the beginning of the bitmap data.
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);

	// Read in the bitmap image data.
	count = fread(bitmapImage, 1, imageSize, filePtr);
	if(count != imageSize)
	{
		return false;
	}

	// Close the file.
	error = fclose(filePtr);
	if(error != 0)
	{
		return false;
	}

	// Create the structure to hold the height map data.
	m_heightMap = new HeightMapType[m_terrainWidth * m_terrainHeight];
	if(!m_heightMap)
	{
		return false;
	}

	// Initialize the position in the image data buffer.
	k=0;

	// Read the image data into the height map.
	for(j=0; j<m_terrainHeight; j++)
	{
		for(i=0; i<m_terrainWidth; i++)
		{
			height = bitmapImage[k];
			
			index = (m_terrainHeight * j) + i;

			m_heightMap[index].x = (float)i;
			m_heightMap[index].y = (float)height;
			m_heightMap[index].z = (float)j;

			k+=3;
		}
	}

	// Release the bitmap image data.
	delete [] bitmapImage;
	bitmapImage = 0;

	return true;
}

void TerrainClass::NormalizeHeightMap()
{
	int i, j;


	for(j=0; j<m_terrainHeight; j++)
	{
		for(i=0; i<m_terrainWidth; i++)
		{
			m_heightMap[(m_terrainHeight * j) + i].y /= 15.0f;
		}
	}

	return;
}

bool TerrainClass::CalculateNormals()
{
	int i, j, index1, index2, index3, index, count;
	float vertex1[3], vertex2[3], vertex3[3], vector1[3], vector2[3], sum[3], length;
	VectorType* normals;


	// Create a temporary array to hold the un-normalized normal vectors.
	normals = new VectorType[(m_terrainHeight-1) * (m_terrainWidth-1)];
	if(!normals)
	{
		return false;
	}

	// Go through all the faces in the mesh and calculate their normals.
	for(j=0; j<(m_terrainHeight-1); j++)
	{
		for(i=0; i<(m_terrainWidth-1); i++)
		{
			index1 = (j * m_terrainHeight) + i;
			index2 = (j * m_terrainHeight) + (i+1);
			index3 = ((j+1) * m_terrainHeight) + i;

			// Get three vertices from the face.
			vertex1[0] = m_heightMap[index1].x;
			vertex1[1] = m_heightMap[index1].y;
			vertex1[2] = m_heightMap[index1].z;
		
			vertex2[0] = m_heightMap[index2].x;
			vertex2[1] = m_heightMap[index2].y;
			vertex2[2] = m_heightMap[index2].z;
		
			vertex3[0] = m_heightMap[index3].x;
			vertex3[1] = m_heightMap[index3].y;
			vertex3[2] = m_heightMap[index3].z;

			// Calculate the two vectors for this face.
			vector1[0] = vertex1[0] - vertex3[0];
			vector1[1] = vertex1[1] - vertex3[1];
			vector1[2] = vertex1[2] - vertex3[2];
			vector2[0] = vertex3[0] - vertex2[0];
			vector2[1] = vertex3[1] - vertex2[1];
			vector2[2] = vertex3[2] - vertex2[2];

			index = (j * (m_terrainHeight-1)) + i;

			// Calculate the cross product of those two vectors to get the un-normalized value for this face normal.
			normals[index].x = (vector1[1] * vector2[2]) - (vector1[2] * vector2[1]);
			normals[index].y = (vector1[2] * vector2[0]) - (vector1[0] * vector2[2]);
			normals[index].z = (vector1[0] * vector2[1]) - (vector1[1] * vector2[0]);
		}
	}

	// Now go through all the vertices and take an average of each face normal 	
	// that the vertex touches to get the averaged normal for that vertex.
	for(j=0; j<m_terrainHeight; j++)
	{
		for(i=0; i<m_terrainWidth; i++)
		{
			// Initialize the sum.
			sum[0] = 0.0f;
			sum[1] = 0.0f;
			sum[2] = 0.0f;

			// Initialize the count.
			count = 0;

			// Bottom left face.
			if(((i-1) >= 0) && ((j-1) >= 0))
			{
				index = ((j-1) * (m_terrainHeight-1)) + (i-1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Bottom right face.
			if((i < (m_terrainWidth-1)) && ((j-1) >= 0))
			{
				index = ((j-1) * (m_terrainHeight-1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper left face.
			if(((i-1) >= 0) && (j < (m_terrainHeight-1)))
			{
				index = (j * (m_terrainHeight-1)) + (i-1);

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}

			// Upper right face.
			if((i < (m_terrainWidth-1)) && (j < (m_terrainHeight-1)))
			{
				index = (j * (m_terrainHeight-1)) + i;

				sum[0] += normals[index].x;
				sum[1] += normals[index].y;
				sum[2] += normals[index].z;
				count++;
			}
			
			// Take the average of the faces touching this vertex.
			sum[0] = (sum[0] / (float)count);
			sum[1] = (sum[1] / (float)count);
			sum[2] = (sum[2] / (float)count);

			// Calculate the length of this normal.
			length = sqrt((sum[0] * sum[0]) + (sum[1] * sum[1]) + (sum[2] * sum[2]));
			
			// Get an index to the vertex location in the height map array.
			index = (j * m_terrainHeight) + i;

			// Normalize the final shared normal for this vertex and store it in the height map array.
			m_heightMap[index].nx = (sum[0] / length);
			m_heightMap[index].ny = (sum[1] / length);
			m_heightMap[index].nz = (sum[2] / length);
		}
	}

	// Release the temporary normals.
	delete [] normals;
	normals = 0;

	return true;
}

void TerrainClass::CalculateTextureCoordinates()
{
	int incrementCount, i, j, tuCount, tvCount;
	float incrementValue, tuCoordinate, tvCoordinate;


	// Calculate how much to increment the texture coordinates by.
	incrementValue = (float)TEXTURE_REPEAT / (float)m_terrainWidth;

	// Calculate how many times to repeat the texture.
	incrementCount = m_terrainWidth / TEXTURE_REPEAT;

	// Initialize the tu and tv coordinate values.
	tuCoordinate = 0.0f;
	tvCoordinate = 1.0f;

	// Initialize the tu and tv coordinate indexes.
	tuCount = 0;
	tvCount = 0;

	// Loop through the entire height map and calculate the tu and tv texture coordinates for each vertex.
	for (j = 0; j<m_terrainHeight; j++)
	{
		for (i = 0; i<m_terrainWidth; i++)
		{
			// Store the texture coordinate in the height map.
			m_heightMap[(m_terrainHeight * j) + i].tu = tuCoordinate;
			m_heightMap[(m_terrainHeight * j) + i].tv = tvCoordinate;

			// Increment the tu texture coordinate by the increment value and increment the index by one.
			tuCoordinate += incrementValue;
			tuCount++;

			// Check if at the far right end of the texture and if so then start at the beginning again.
			if (tuCount == incrementCount)
			{
				tuCoordinate = 0.0f;
				tuCount = 0;
			}
		}

		// Increment the tv texture coordinate by the increment value and increment the index by one.
		tvCoordinate -= incrementValue;
		tvCount++;

		// Check if at the top of the texture and if so then start at the bottom again.
		if (tvCount == incrementCount)
		{
			tvCoordinate = 1.0f;
			tvCount = 0;
		}
	}

	return;
}

bool TerrainClass::LoadTextures(ID3D11Device* device, WCHAR* grassTextureFilename, WCHAR* slopeTextureFilename, WCHAR* rockTextureFilename)
{
	bool result;


	// Create the grass texture object.
	m_GrassTexture = new TextureClass;
	if (!m_GrassTexture)
	{
		return false;
	}

	// Initialize the grass texture object.
	result = m_GrassTexture->Initialize(device, grassTextureFilename);
	if (!result)
	{
		return false;
	}

	// Create the slope texture object.
	m_SlopeTexture = new TextureClass;
	if (!m_SlopeTexture)
	{
		return false;
	}

	// Initialize the slope texture object.
	result = m_SlopeTexture->Initialize(device, slopeTextureFilename);
	if (!result)
	{
		return false;
	}

	// Create the rock texture object.
	m_RockTexture = new TextureClass;
	if (!m_RockTexture)
	{
		return false;
	}

	// Initialize the rock texture object.
	result = m_RockTexture->Initialize(device, rockTextureFilename);
	if (!result)
	{
		return false;
	}

	return true;
}

void TerrainClass::ReleaseTextures()
{
	// Release the texture objects.
	if (m_GrassTexture)
	{
		m_GrassTexture->Shutdown();
		delete m_GrassTexture;
		m_GrassTexture = 0;
	}

	if (m_SlopeTexture)
	{
		m_SlopeTexture->Shutdown();
		delete m_SlopeTexture;
		m_SlopeTexture = 0;
	}

	if (m_RockTexture)
	{
		m_RockTexture->Shutdown();
		delete m_RockTexture;
		m_RockTexture = 0;
	}

	return;
}

void TerrainClass::ShutdownHeightMap()
{
	if(m_heightMap)
	{
		delete [] m_heightMap;
		m_heightMap = 0;
	}

	return;
}

bool TerrainClass::InitializeBuffers(ID3D11Device* device)
{
	VertexType* vertices;
	unsigned long* indices;
	int index, i, j;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
    D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;
	int index1, index2, index3, index4;
	float tu, tv;


	// Calculate the number of vertices in the terrain mesh.
	m_vertexCount = (m_terrainWidth - 1) * (m_terrainHeight - 1) * 6;

	// Set the index count to the same as the vertex count.
	m_indexCount = m_vertexCount;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];
	if(!vertices)
	{
		return false;
	}

	// Create the index array.
	indices = new unsigned long[m_indexCount];
	if(!indices)
	{
		return false;
	}

	// Initialize the index to the vertex buffer.
	index = 0;

	// Load the vertex and index array with the terrain data.
	// Load the vertex and index array with the terrain data.
	for (j = 0; j<(m_terrainHeight - 1); j++)
	{
		for (i = 0; i<(m_terrainWidth - 1); i++)
		{
			index1 = (m_terrainHeight * j) + i;          // Bottom left.
			index2 = (m_terrainHeight * j) + (i + 1);      // Bottom right.
			index3 = (m_terrainHeight * (j + 1)) + i;      // Upper left.
			index4 = (m_terrainHeight * (j + 1)) + (i + 1);  // Upper right.

															 // Upper left.
			tv = m_heightMap[index3].tv;

			// Modify the texture coordinates to cover the top edge.
			if (tv == 1.0f) { tv = 0.0f; }

			vertices[index].position = D3DXVECTOR3(m_heightMap[index3].x, m_heightMap[index3].y, m_heightMap[index3].z);
			vertices[index].texture = D3DXVECTOR2(m_heightMap[index3].tu, tv);
			vertices[index].normal = D3DXVECTOR3(m_heightMap[index3].nx, m_heightMap[index3].ny, m_heightMap[index3].nz);
			indices[index] = index;
			index++;

			// Upper right.
			tu = m_heightMap[index4].tu;
			tv = m_heightMap[index4].tv;

			// Modify the texture coordinates to cover the top and right edge.
			if (tu == 0.0f) { tu = 1.0f; }
			if (tv == 1.0f) { tv = 0.0f; }

			vertices[index].position = D3DXVECTOR3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].texture = D3DXVECTOR2(tu, tv);
			vertices[index].normal = D3DXVECTOR3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = D3DXVECTOR3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].texture = D3DXVECTOR2(m_heightMap[index1].tu, m_heightMap[index1].tv);
			vertices[index].normal = D3DXVECTOR3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			indices[index] = index;
			index++;

			// Bottom left.
			vertices[index].position = D3DXVECTOR3(m_heightMap[index1].x, m_heightMap[index1].y, m_heightMap[index1].z);
			vertices[index].texture = D3DXVECTOR2(m_heightMap[index1].tu, m_heightMap[index1].tv);
			vertices[index].normal = D3DXVECTOR3(m_heightMap[index1].nx, m_heightMap[index1].ny, m_heightMap[index1].nz);
			indices[index] = index;
			index++;

			// Upper right.
			tu = m_heightMap[index4].tu;
			tv = m_heightMap[index4].tv;

			// Modify the texture coordinates to cover the top and right edge.
			if (tu == 0.0f) { tu = 1.0f; }
			if (tv == 1.0f) { tv = 0.0f; }

			vertices[index].position = D3DXVECTOR3(m_heightMap[index4].x, m_heightMap[index4].y, m_heightMap[index4].z);
			vertices[index].texture = D3DXVECTOR2(tu, tv);
			vertices[index].normal = D3DXVECTOR3(m_heightMap[index4].nx, m_heightMap[index4].ny, m_heightMap[index4].nz);
			indices[index] = index;
			index++;

			// Bottom right.
			tu = m_heightMap[index2].tu;

			// Modify the texture coordinates to cover the right edge.
			if (tu == 0.0f) { tu = 1.0f; }

			vertices[index].position = D3DXVECTOR3(m_heightMap[index2].x, m_heightMap[index2].y, m_heightMap[index2].z);
			vertices[index].texture = D3DXVECTOR2(tu, m_heightMap[index2].tv);
			vertices[index].normal = D3DXVECTOR3(m_heightMap[index2].nx, m_heightMap[index2].ny, m_heightMap[index2].nz);
			indices[index] = index;
			index++;
		}
	}


	// Set up the description of the static vertex buffer.
    vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
    vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
    result = device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer);
	if(FAILED(result))
	{
		return false;
	}

	// Set up the description of the static index buffer.
    indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
    indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	result = device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer);
	if(FAILED(result))
	{
		return false;
	}

	// Release the arrays now that the buffers have been created and loaded.
	delete [] vertices;
	vertices = 0;

	delete [] indices;
	indices = 0;

	return true;
}

void TerrainClass::ShutdownBuffers()
{
	// Release the index buffer.
	if(m_indexBuffer)
	{
		m_indexBuffer->Release();
		m_indexBuffer = 0;
	}

	// Release the vertex buffer.
	if(m_vertexBuffer)
	{
		m_vertexBuffer->Release();
		m_vertexBuffer = 0;
	}

	return;
}

void TerrainClass::RenderBuffers(ID3D11DeviceContext* deviceContext)
{
	unsigned int stride;
	unsigned int offset;


	// Set vertex buffer stride and offset.
	stride = sizeof(VertexType); 
	offset = 0;
    
	// Set the vertex buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

    // Set the index buffer to active in the input assembler so it can be rendered.
	deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

    // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	return;
}