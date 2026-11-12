using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Science.Formal.Mathematics.Geometry.Topology;
using Ideam.Science.Formal.Mathematics.Geometry.Topology.Tiling;


namespace Ideam.Unity.Science.Applied.ProceduralGeneration
{
    public class ProceduralWallpaper : MonoBehaviour
    {

        public ComplexSymmetryGroup wallpaper;
        public SymmetryGroupType wallpaperType;

        public Spline2[] regionsToWallpaper = new Spline2[0];

        public ProceduralAsset wallpaperAssets, symmetryAssets, edgeAssets, pointAssets;


        public bool[] keepInstance = new bool[0],
                      keepSymmetryInstance = new bool[0],
                      keepEdgeInstance = new bool[0],
                      keepPointInstance = new bool[0];

        public enum WallpaperPhase
        {
            Uninitialized, RegionSetup, WallpaperSetup, AssetDistribution, Complete
        }

        [SerializeField]
        protected WallpaperPhase phase;

        public WallpaperPhase Phase 
        {
            get 
            {
                if(regionsToWallpaper.Length == 0) { phase = WallpaperPhase.Uninitialized; }
                else if(wallpaper.NodesCount ==0) { phase = WallpaperPhase.RegionSetup; }
                // else if(wallpaperInstances.Length == 0) { phase = WallpaperPhase.WallpaperSetup; }
                return phase;
            }
            set
            {
                phase = value;
            }
        }

        public void Wallpaper()
        {
            wallpaper = ComplexSymmetryGroup.Wallpaper(wallpaperType, regionsToWallpaper, Float2.zero, OnPaper);
        }

        private void OnPaper(IEnumerator obj)
        {
            StartCoroutine(PaperRoutine(obj));
        }

        public IEnumerator PaperRoutine(IEnumerator obj)
        {
            while (!obj.MoveNext()) { yield return null; }
        }

        public IEnumerator PopulateWallpaper()
        {
            ValidateInstances();

            IEnumerator populator = PopulateEdges();
            while (populator.MoveNext()) { yield return null; }

            populator = PopulateSymmetries();
            while(populator.MoveNext()) { yield return null; }

            populator = PopulatePoints();
            while(populator.MoveNext()) { yield return null; }


            

            /*
            for (int i = 0; i < wallpaperInstances.Length; i++)
            {
                if (!keepInstance[i])
                {
                    if (wallpaperInstances[i] != null && wallpaperInstances[i].gameObjectInstance != null)
                    { DestroyImmediate(wallpaperInstances[i].gameObjectInstance); }

                    wallpaperInstances[i] = wallpaperAssets.InstantiateGameObject(UnityEngine.Random.Range(0, wallpaperAssets.assets.Length));

                    IsoSymmetry sym = wallpaper.symmetries[i];

                    Transform iT = wallpaperInstances[i].gameObjectInstance.transform;
                    iT.SetParent(transform, true);
                    switch (wallpaper.axisConversion)
                    {
                        case XYToXYZ.XY:
                            iT.localScale = new Vector3(sym.Scale.x, sym.Scale.y, 1);
                            iT.localEulerAngles = new Vector3(0, 0, sym.Angle);
                            break;
                        case XYToXYZ.XZ:
                            iT.localScale = new Vector3(sym.Scale.x, 1, sym.Scale.y);
                            iT.localEulerAngles = new Vector3(0, sym.Angle, 0);
                            break;
                        case XYToXYZ.YZ:
                            iT.localScale = new Vector3(1, sym.Scale.x, sym.Scale.y);
                            iT.localEulerAngles = new Vector3(sym.Angle, 0, 0);
                            break;
                    }
                    iT.localPosition = sym.Position.ToVector3(wallpaper.axisConversion);

                    yield return null;
                }
            }
            */

           



            yield break;
        }

        public IEnumerator PopulateSymmetries()
        {
            /*
            for (int i = 0; i < symmetryInstances.Length; i++)
            {
                if (!keepSymmetryInstance[i])
                {
                    if (symmetryInstances[i] != null && symmetryInstances[i].gameObjectInstance != null)
                    { DestroyImmediate(symmetryInstances[i].gameObjectInstance); }

                    symmetryInstances[i] = wallpaperAssets.InstantiateGameObject(UnityEngine.Random.Range(0, symmetryAssets.assets.Length));

                    IsoSymmetry sym = wallpaper.symmetries[i];

                    Transform iT = symmetryInstances[i].gameObjectInstance.transform;
                    iT.SetParent(transform, true);
                    switch (wallpaper.axisConversion)
                    {
                        case XYToXYZ.XY:
                            iT.localScale = new Vector3(sym.Scale.x, sym.Scale.y, 1);
                            iT.localEulerAngles = new Vector3(0, 0, sym.Angle);
                            break;
                        case XYToXYZ.XZ:
                            iT.localScale = new Vector3(sym.Scale.x, 1, sym.Scale.y);
                            iT.localEulerAngles = new Vector3(0, sym.Angle, 0);
                            break;
                        case XYToXYZ.YZ:
                            iT.localScale = new Vector3(1, sym.Scale.x, sym.Scale.y);
                            iT.localEulerAngles = new Vector3(sym.Angle, 0, 0);
                            break;
                    }
                    iT.localPosition = sym.Position.ToVector3(wallpaper.axisConversion);

                    yield return null;
                }
            }
            */
            yield break;
        }

        public IEnumerator PopulateEdges()
        {
            /*
            for (int i = 0; i < edgeInstances.Length; i++)
            {
                if (!keepEdgeInstance[i])
                {
                    if (edgeInstances[i] != null && edgeInstances[i].gameObjectInstance != null)
                    { DestroyImmediate(edgeInstances[i].gameObjectInstance); }

                    edgeInstances[i] = wallpaperAssets.InstantiateGameObject(UnityEngine.Random.Range(0, edgeAssets.assets.Length));

                    SymmetryEdge edge = wallpaper.edges[i];

                    Transform iT = edgeInstances[i].gameObjectInstance.transform;
                    iT.SetParent(transform, true);

                    //How do I get the scale and rotation?

                    iT.localPosition = ((wallpaper.vertices[edge.NodeA] + wallpaper.vertices[edge.NodeB]) * 0.5f).ToVector3(wallpaper.axisConversion);

                    yield return null;
                }
            }
            */
            yield break;
        }

        public IEnumerator PopulatePoints()
        {
            /*
            for (int i = 0; i < pointInstances.Length; i++)
            {
                if (!keepPointInstance[i])
                {
                    if (pointInstances[i] != null && pointInstances[i].gameObjectInstance != null)
                    { DestroyImmediate(pointInstances[i].gameObjectInstance); }

                    pointInstances[i] = wallpaperAssets.InstantiateGameObject(UnityEngine.Random.Range(0, wallpaperAssets.assets.Length));

                    Float2 point = wallpaper.vertices[i];

                    Transform iT = pointInstances[i].gameObjectInstance.transform;
                    iT.SetParent(transform, true);
                    
                    //Again, how do I find rotation and scale for these assets?

                    iT.localPosition = point.ToVector3(wallpaper.axisConversion);

                    yield return null;
                }
            }
            */
            yield break;
        }

        public void ValidateInstances()
        {
            int nc = wallpaper.NodesCount, ec = wallpaper.EdgesCount, pc = wallpaper.Vertices.Length;

            /*
            if (wallpaperInstances.Length != nc)
            {
                //Release old assets
                for (int i = 0; i < wallpaperInstances.Length; i++)
                {
                    if (i > nc) { DestroyImmediate(wallpaperInstances[i].gameObjectInstance); }
                    else
                    {
                        if (!keepInstance[i])
                        {
                            DestroyImmediate(wallpaperInstances[i].gameObjectInstance);
                        }
                    }
                }
                System.Array.Resize(ref wallpaperInstances, nc);
                System.Array.Resize(ref keepInstance, nc);

            }
            

            if (symmetryInstances.Length != nc)
            {
                //Release old assets
                for (int i = 0; i < symmetryInstances.Length; i++)
                {
                    if (i > nc) { DestroyImmediate(symmetryInstances[i].gameObjectInstance); }
                    else
                    {
                        if (!keepInstance[i])
                        {
                            DestroyImmediate(symmetryInstances[i].gameObjectInstance);
                        }
                    }
                }
                System.Array.Resize(ref symmetryInstances, nc);
                System.Array.Resize(ref keepSymmetryInstance, nc);

            }

            if (edgeInstances.Length != ec)
            {
                //Release old assets
                for (int i = 0; i < edgeInstances.Length; i++)
                {
                    if (i > ec) { DestroyImmediate(edgeInstances[i].gameObjectInstance); }
                    else
                    {
                        if (!keepInstance[i])
                        {
                            DestroyImmediate(edgeInstances[i].gameObjectInstance);
                        }
                    }
                }
                System.Array.Resize(ref edgeInstances, ec);
                System.Array.Resize(ref keepEdgeInstance, ec);

            }

            if (pointInstances.Length != pc)
            {
                //Release old assets
                for (int i = 0; i < pointInstances.Length; i++)
                {
                    if (i > pc) { DestroyImmediate(pointInstances[i].gameObjectInstance); }
                    else
                    {
                        if (!keepInstance[i])
                        {
                            DestroyImmediate(pointInstances[i].gameObjectInstance);
                        }
                    }
                }
                System.Array.Resize(ref pointInstances, pc);
                System.Array.Resize(ref keepPointInstance, pc);

            }
            */
        }
    }
}