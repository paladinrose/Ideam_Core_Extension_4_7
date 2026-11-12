using System;
using System.Collections;
using System.Collections.Generic;

using Ideam.Science.Natural.Physics;
using Ideam.Metaphysics.Ontology.Existence;
using Ideam.Axiology.Aesthetics.The_Arts.Narratology;
using Ideam.Axiology.Aesthetics.The_Arts.Ludology;

namespace Ideam.Science.Natural.Cosmology
{
    /// <summary>
    /// A cosmos is defined as an ordered, harmonious whole. In this case, it represents
    /// root level access for many similar, but distinct big-picture structures: Realities,
    /// Imaginations, Universes, and Narratives, as just a few examples.
    /// </summary>
    public interface ICosmos
    {
        T GetUniverse<T>(int id) where T : IUniverse;
        T[] GetUniverses<T>() where T : IUniverse;

        T GetReality<T>(int id) where T : IReality;
        T[] GetRealities<T>() where T : IReality;

        T GetImagination<T>(int id) where T : IImagination;
        T[] GetImaginations<T>() where T : IImagination;

        T GetNarrative<T>(int id) where T : INarrative;
        T[] GetNarratives<T>() where T : INarrative;

        T GetGame<T>(int id) where T : IGame;
        T[] GetGames<T>() where T : IGame;
    }
}