using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using System;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology.Gameplay.CharacterControls
{
    [System.Serializable]
    public class BlendShapesGesture
    {

        public Spline1[] blendShapeSplines = new Spline1[0];
        public int[] blendShapeIDs = new int[0];
        public string[] blendShapeNames = new string[0];

        [SerializeField]
        protected SkinnedMeshRenderer skin;

        public SkinnedMeshRenderer Skin { get { return skin; } set { if (value != skin) { SetSkin(value); } } }
        public BlendShapesGesture(SkinnedMeshRenderer skin)
        {
            if(skin != null)
            SetSkin(skin);
        }

        public void SetSkin(SkinnedMeshRenderer newSkin)
        {
            skin = newSkin;
            int blendShapeCount = skin.sharedMesh.blendShapeCount;
            
            List<Spline1> newBlendSplines = new List<Spline1>(blendShapeSplines);
            List<int> newBlendShapeIDs = new List<int>(blendShapeIDs);
            List<string> newBlendShapeNames = new List<string>(blendShapeNames);

            for(int i = newBlendSplines.Count-1; i >=0; i--)
            {
                if(blendShapeIDs[i] >= blendShapeCount)
                {
                    newBlendShapeIDs.RemoveAt(i);
                    newBlendSplines.RemoveAt(i);
                    newBlendShapeNames.RemoveAt(i);
                }
                else
                {
                    newBlendShapeNames[i] = skin.sharedMesh.GetBlendShapeName(newBlendShapeIDs[i]);
                }
            }

            blendShapeIDs = newBlendShapeIDs.ToArray();
            blendShapeSplines = newBlendSplines.ToArray();
            blendShapeNames = newBlendShapeNames.ToArray();

        }

        public int AddBlendShape(int blendShapeID, Spline1 blendSpline)
        {
            if (blendShapeID >= skin.sharedMesh.blendShapeCount || blendShapeID < 0) { return -1; }

            int bsid = GetBlendShapeLocalID(blendShapeID);
            if (bsid >= 0) { blendShapeSplines[bsid] = blendSpline; return bsid; }

            int l = blendShapeIDs.Length;
            System.Array.Resize(ref blendShapeSplines, l + 1);
            System.Array.Resize(ref blendShapeIDs, l + 1);
            System.Array.Resize(ref blendShapeNames, l + 1);

            blendShapeSplines[l] = blendSpline;
            blendShapeIDs[l] = blendShapeID;
            blendShapeNames[l] = skin.sharedMesh.GetBlendShapeName(blendShapeID);

            return l;
        }

        public int AddBlendShape(string blendShapeName, Spline1 blendSpline)
        {
            int localID = GetBlendShapeLocalID(blendShapeName);
            if(localID >= 0) { return localID; }
            else
            {
                return AddBlendShape(skin.sharedMesh.GetBlendShapeIndex(blendShapeName), blendSpline); 
                
            }
        }

        public int GetBlendShapeLocalID(string blendShapeName)
        {
            for (int i = 0; i < blendShapeNames.Length; i++)
            {
                if (blendShapeNames[i] == blendShapeName) { return i; }
            }
            return -1;
        }
        public int GetBlendShapeLocalID(int blendShapeID)
        {
            for(int i =0; i < blendShapeIDs.Length; i++)
            {
                if(blendShapeIDs[i] == blendShapeID) { return i; }
            }
            return -1;
        }

        public void RemoveBlendShape(int id, bool idIsLocal = true)
        {

            if (!idIsLocal) { id = GetBlendShapeLocalID(id); }

            if (id < 0 || id >= blendShapeIDs.Length) return;

            List<Spline1> newSplines = new List<Spline1>(blendShapeSplines);
            List<int> newIDs = new List<int>(blendShapeIDs);
            List<string> newNames = new List<string>(blendShapeNames);

            newSplines.RemoveAt(id);
            newIDs.RemoveAt(id);
            newNames.RemoveAt(id);
            
            blendShapeSplines = newSplines.ToArray();
            blendShapeIDs = newIDs.ToArray();
            blendShapeNames = newNames.ToArray();
        }

        public void Gesture(float t)
        {
            for (int i = 0; i < blendShapeSplines.Length; i++)
            {
                float v = blendShapeSplines[i].GetPoint(t);
                skin.SetBlendShapeWeight(blendShapeIDs[i], v * 100f);
            }
        }
    }
}
