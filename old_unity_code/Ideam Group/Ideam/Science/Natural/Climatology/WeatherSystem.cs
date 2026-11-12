using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class WeatherSystem : MonoBehaviour
{
    struct WeatherVoxel
    {
        public float temperature,
                     pressure, humidity;
        public Vector3 windNormal;
        
    }
    public Vector3Int voxelCounts;
    WeatherVoxel[,,] weatherVoxels;

    private void Start()
    {
        weatherVoxels = new WeatherVoxel[voxelCounts.x, voxelCounts.y, voxelCounts.z];
    }
    
    WeatherVoxel GetVoxel(int x, int y, int z)
    {
        
        return weatherVoxels[x, y, z];
    }

    WeatherVoxel GetVoxel(Vector3Int voxelCoords)
    {
        return weatherVoxels[voxelCoords.x, voxelCoords.y, voxelCoords.z];
    }
}


/*
Each voxel has temperature, pressure, and humidity. They also hold local wind direction.
When we pass through the voxels, we ask several questions to answer several conditions:
1. Is the pressure in the voxel above this one higher than in this one? If so, apply a downward force to the wind direction as the pressure 
   tries to fall. Raise the pressure in the lower voxel by the same amount we lower the pressure in this voxel.
2. Is the temperature in the voxel above this one lower than this one? If so, apply an upward force as the heat rises. Raise the temperature
   in the voxel above this one by the same amount we lower the temperature in this voxel.
3. Create a 2D Ground Map and Sky Map that are simple flattened values representing the ground below the voxels and the sky above the voxels.
4. When you have an updraft - a column of voxels x wide and z long with hot air moving up - and a downdraft next to one another, 
   that's a 'cell'.
5. A 'super cell' is when all the updrafts and downdrafts in a large storm coalesce into one updraft and one downdraft. 
6. The updraft has the appearance of clouds roiling forward and billowing upward. The downdraft is the rain.
7. The wind gets stronger with altitude. This tilts the updraft in the direction(s) of the wind.
8. Because of the tilt, the rain - the downdraft - is pushed out and down in the direction of the tilt. It doesn't merge
   with the updraft and neutralize the whole thing. This is why supercells can last for hours and hours, while a strong storm is typically
   a half hour.
9. An updraft implicitly creates a downdraft -as the air rises, it's pressure (and temperature) fall and its humidity rises. 
   Once a voxel's pressure and temp drop below those below it, it starts to exchange heat the opposite way - it starts to fall.
10. The Coreolis Effect is what creates the spin of the updraft. There's essentially more drag on wind directions that are moving 'counter'
    to the rotation of the Earth. Since one 'side' is slower, the pressure on the other can equalize by turning in that direction.
    This extra force turns the vector of the 'slow side' farther, which lowers the force pushing against it, allowing it to speed up.
*/