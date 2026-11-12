using System;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Science.Applied.Architecture;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Unity.Science.Applied.Architecture
{
    public class Aperture : StructurePiece, IAperture
    {
        public Spline2 apertureShape;
        public Surface[] surfaces;
        public UnityGamePlace[] linkedPlaces;

        public virtual bool IsOpen { get { return isOpen; } }

        public virtual event Action OnOpen;
        public virtual event Action OnClose;


        protected bool isOpen, isLocked;

        public virtual bool Close()
        {
            throw new NotImplementedException();
        }

        public virtual bool Open()
        {
            throw new NotImplementedException();
        }

        public virtual T[] Surfaces<T>() where T : ISurface
        {
            List<T> tSurfs = new List<T>();
            for(int i =0; i < surfaces.Length; i++) 
            {
                if(surfaces[i] is T tSurf) { tSurfs.Add(tSurf); }
            }
            return tSurfs.ToArray();

        }

        public virtual T[] LinkedPlaces<T>() where T : IPlace
        {
            List<T> tPlaces = new List<T>();
            for (int i = 0; i < linkedPlaces.Length; i++)
            {
                if (linkedPlaces[i] is T tPlace) { tPlaces.Add(tPlace); }
            }
            return tPlaces.ToArray();
        }

        public virtual void UpdateAperture()
        {


            //Lastly (Maybe firstly, I'm not sure if it matters...) , we tell the surface that this door is in to resize its aperture hole.
            for (int i = 0; i < surfaces.Length; i++)
            {
                surfaces[i].ApertureUpdated(this);
            }

        }
    }
}