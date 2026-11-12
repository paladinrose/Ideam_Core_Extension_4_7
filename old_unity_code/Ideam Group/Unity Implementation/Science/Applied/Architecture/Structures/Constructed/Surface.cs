using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Applied.Architecture;
using Ideam.Science.Formal;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Unity.Science.Applied.Architecture
{
    public class Surface : StructurePiece, ISurface
    {
        public float width, height;
        public Spline2 surfaceShape;
        public Aperture[] apertures;

        public MeshRenderer meshRenderer;
        public MeshFilter meshFilter;

        protected Mesh mesh;

        
        public T[] Apertures<T>() where T : IAperture
        {
            throw new System.NotImplementedException();
        }

        public T GetAperture<T>(int id) where T : IAperture
        {
            throw new System.NotImplementedException();
        }

        public void ApertureUpdated<T>(T aperture)
        {
            throw new System.NotImplementedException();
        }
        
        public void UpdateSurface()
        {
            Float2 size = new Float2(width, height);
            Float2 center = size / 2;
            
            Spline2 surfaceRect = Spline2.NewFromRect(new Bounds2(center, size));

            int a = apertures.Length;
            Spline2[] apertureShapes = new Spline2[a];
            for (int i = 0; i < a; i++)
            {
                apertureShapes[i] = new Spline2(apertures[i].apertureShape);
                apertureShapes[i].ScaleSpline(size);
                apertureShapes[i].Offset(center);
            }

        }
    }
}