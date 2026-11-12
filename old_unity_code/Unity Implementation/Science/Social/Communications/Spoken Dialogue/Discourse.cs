using System;
using System.Collections;
using System.Collections.Generic;


using UnityEngine;


using Ideam.Epistemology.Semantics;

using Ideam.Science.Formal.Linguistics;
using Ideam.Science.Formal.Graphs;
using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Geometry.Splines;
using Ideam.Science.Formal.Mathematics.Information;

using Ideam.Axiology.Valuation;

using Ideam.Unity.Metaphysics.Mind;
using Ideam.Unity.Metaphysics.Mind.Perception;

namespace Ideam.Unity.Science.Social.Communications
{
    public class Discourse : ScriptableObject
    {
        
        public event Action<Discourse, UnityMind> OnEnterDiscourse, OnLeaveDiscourse;
        public DiscourseMedium medium;
        public DiscourseGraph discourseGraph;

        public UnityMind[] mindsInDiscourse = new UnityMind[0];

        public int currentTurn = -1;

        public int EnterDiscourse(UnityMind mind)
        {
            int mindID = GetUnityMindID(mind);
            if (mindID < 0)
            {
                mindID = mindsInDiscourse.Length;
                System.Array.Resize(ref mindsInDiscourse, mindID + 1);
                mindsInDiscourse[mindID] = mind;
            }
            return mindID;
        }

        public int GetUnityMindID(UnityMind mind)
        {
            for(int i =0; i < mindsInDiscourse.Length; i++)
            {
                if(mind == mindsInDiscourse[i]) { return i; }
            }
            return -1;
        }
      

        public bool LeaveDiscourse(UnityMind mind)
        {
            int mindID = GetUnityMindID(mind);
            if (mindID < 0) { return false; }

            List<UnityMind> newUnityMinds = new List<UnityMind>(mindsInDiscourse);
            newUnityMinds.RemoveAt(mindID);
            
            mindsInDiscourse = newUnityMinds.ToArray();
            
            return true;
        }



        public void UpdateTurn(float delta) 
        {
            Turn turn = discourseGraph.GetTurn(currentTurn);

            turn.Update(delta);

            for(int i =0; i < mindsInDiscourse.Length; i++)
            {
                for(int j =0; j < turn.speakers.Length; j++)
                {
                    Speaker speaker = turn.speakers[j];

                    //Hold the phone! Do I want speakers to listen to themselves? For what? How?
                    if(mindsInDiscourse[i] == speaker.mind) { continue; }

                    Utterance speakerUtterance = turn.speechData[j];
                }
            }
        }

        protected void TurnAllocation() 
        {
            //a. If the current speaker selects a next one to speak at the end of current turn (by name, gaze or contextual aspects of what is said),
            //   the selected speaker has the right and obligation to speak next.
            //b. If the current speaker does not select a next speaker, other potential speakers have the right to self - select(the first starter gets the turn)
            //c. If options 1a and 1b have not been implemented, current speaker may continue with another TCU.
            // At the end of that turn, the option system applies again
        }


    }

    [Serializable]
    public class DiscourseGraph : IGraph
    {
        public Turn[] turns = new Turn[0];
        public Transition[] transitions = new Transition[0];

        public int NodesCount { get { return turns.Length; } }
        public int EdgesCount { get { return transitions.Length; } }

        public int AddEdge<T>(T edge) where T : IEdge
        {
           if(edge is Transition tEdge)
            {
               return AddTransition(tEdge);
            }
            return -1;
        }

        public int AddTransition(Transition transition)
        {
            throw new NotImplementedException();
        }

        public int AddNode<T>(T node) where T : INode
        {
            if(node is Turn turn) { return AddTurn(turn); }
            return -1;
        }

        public int AddTurn(Turn turn)
        {
            throw new NotImplementedException();
        }

        public T GetEdge<T>(int id) where T : IEdge
        {
            if(transitions[id] is T tTrans) { return tTrans; }
            return default;
        }
        
        public T[] GetEdges<T>() where T : IEdge
        {
            List<T> trans = new List<T>();
            for(int i =0; i < transitions.Length; i++)
            {
                if(transitions[i] is T tTrans) { trans.Add(tTrans); }
            }
            return trans.ToArray();
        }

        public T GetNode<T>(int id) where T : INode
        {
            if(turns[id] is T tTurn) { return tTurn; }
            return default;
        }
        public Turn GetTurn(int id)
        {
            if(id >=0 && id < turns.Length) { return turns[id]; }
            return null;
        }
        public T[] GetNodes<T>() where T : INode
        {
            List<T> trns = new List<T>();
            for (int i = 0; i < turns.Length; i++)
            {
                if (turns[i] is T tTurn) { trns.Add(tTurn); }
            }
            return trns.ToArray();
        }

        public int[] GraphSearch(int startCell, int endCell)
        {
            throw new NotImplementedException();
        }

        public void RemoveEdge(int localID)
        {
            throw new NotImplementedException();
        }

        public void RemoveNode(int localID)
        {
            throw new NotImplementedException();
        }
    }

    [Serializable]
    public class Turn: INode 
    {
        public Speaker[] speakers;
        public TurnConstructionUnit[] constructionUnits;
        public Utterance[] speechData;

        public Float2 position;
        public Float2 Position { get { return position; } }

        public T[] GetPorts<T>() where T : IPort
        {
            return new T[0];
        }

        public void SetPosition(Float2 newPosition)
        {
            position = newPosition;
        }

        internal void Update(Float delta)
        {
            for(int i=0; i < speakers.Length; i++)
            {
                speakers[i].Transmit(speechData[i], delta);
            }
        }
    }

    [Serializable]
    public class TurnPort : IPort
    {
        public int MaximumConnections { get; set; }
        public PortDirection Direction { get; set; }

        public Type AcceptedType()
        {
            throw new NotImplementedException();
        }
    }

    [Serializable]
    public class TurnConstructionUnit
    {
        public TurnConstructionUnitType type;
        public TurnResolutionType resolution;
    }

    public enum TurnConstructionUnitType
    {
        Undefined,
        Lexical,
        Phrasal,
        Clausal,
        Sentential
    }

    public enum TurnResolutionType
    {
        /// <summary>
        /// The Turn is not yet resolved
        /// </summary>
        Unresolved,
        /// <summary>
        /// The Turn resolved through a shift in prosody, such as pitch or volume.
        /// </summary>
        Intonational,
        /// <summary>
        /// The Turn resolved through word-usage. Interestingly, it SHOULD be grammatical, not grammatic.
        /// </summary>
        Grammatical,
        /// <summary>
        /// The Turn was resolved practically, usually through situational interference. Don't @ me, it should be pragmatical, not pragmatic. I checked.
        /// </summary>
        Pragmatical
    }

    [Serializable]
    public class Transition : IEdge
    {
        public TransitionType transitionType;

        public int NodeA { get; set; }
        public int PortA { get; set; }
        public int NodeB { get; set; }
        public int PortB { get; set; }
    }

    public enum TransitionType
    {
        Undefined,
        Grammatical,
        Prosodic,
        Pragmatic,
        NonVerbal
    }

    [Serializable]
    public class Utterance : UnityAudioSignal
    {
        [SerializeField]
        protected Float currentVolume, currentPitch;
        
        public UnitySentence prose;
        
        //X = Volume
        //Y = Pitch
        public Spline2 prosody;
        
       

        public void Update(Float delta) 
        {
            percentTransmitted += delta;
        }

    }
}