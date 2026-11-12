using System;
using System.Collections.Generic;

using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Science.Formal.Mathematics.Information;

using Ideam.Metaphysics.Mind;
using Ideam.Metaphysics.Mind.Perception;
using Ideam.Axiology.Valuation;

using Ideam.Unity.Semantics;
using Ideam.Unity.Metaphysics.Mind.Perception;
using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.Axiology.Aesthetics.The_Arts.Narratology;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology;
using Ideam.Unity.Science.Social.Communications;

namespace Ideam.Unity.Metaphysics.Mind
{
    public class UnityMind : MonoBehaviour, IMind
    {
        public UnityGameAgent agent;
        public UnityCharacter character;
        public InformationMatrix memory;
        public UnitySemanticManager semanticManager;
        public UnityLanguageAsset[] languages;

        #region Monobehaviour Implementation

        protected virtual void OnEnable()
        {
            if(sight != null) sight.EnableSensing();
            if(hearing != null) hearing.EnableSensing();
            if(touch != null) touch.EnableSensing();
            if (interoception != null) interoception.EnableSensing();
        }

        protected virtual void OnDisable()
        {
            if(sight != null) sight.DisableSensing();
            if(hearing != null) hearing.DisableSensing();
            if(touch != null) touch.DisableSensing();
            if (interoception != null) interoception.DisableSensing();
        }

        #endregion

        #region Cognition
        public event Action<UnityMind> OnStartCognitiveUpdate, OnCompleteCognitiveUpdate;
        public Int currentAttention;
        public UnityMentalState[] mentalStates;
        public UnityEmotion[] emotions;

        protected List<Information> newSenseInfo, shortTermMemory;
        protected List<StimulusPriority> latentPriorities;

        public int GetEmotionID<T>(T emotion) where T : IEmotion
        {
            if(emotion is UnityEmotion uMS) { return GetEmotionID(uMS); }
            return -1;
        }

        public int GetEmotionID(UnityEmotion uMS)
        {
            for(int i =0; i < emotions.Length; i++)
            {
                if(emotions[i] == uMS) { return i; }
            }
            return -1;
        }

        public T GetEmotion<T>(int id) where T : IEmotion
        {
            if(id >=0 && id < emotions.Length)
            {
                if(emotions[id] is T tState) { return tState; }
            }
            return default;
        }

        public int GetMentalStateID<T>(T mentalState) where T : IMentalState
        {
            if (mentalState is UnityMentalState uMS) { return GetMentalStateID(uMS); }
            return -1;
        }

        public int GetMentalStateID(UnityMentalState uMS)
        {
            for (int i = 0; i < mentalStates.Length; i++)
            {
                if (mentalStates[i] == uMS) { return i; }
            }
            return -1;
        }

        public T GetMentalState<T>(int id) where T : IMentalState
        {
            if (id >= 0 && id < mentalStates.Length)
            {
                if (mentalStates[id] is T tState) { return tState; }
            }
            return default;
        }

        
        public virtual void CognitiveUpdate()
        {
            OnStartCognitiveUpdate?.Invoke(this);

            SensoryInput();

            Latency();

            EmotionalResponse();

            AlterMentalState();

            //5. Respond cognitively:
            //   -Evaluate emotions, and mental states
            //   -Prioritize shortTermMemory things against Goals, then sort based on Resource Cost and Risk, Mores, and Taboos


            OnCompleteCognitiveUpdate?.Invoke(this);
        }

        /// <summary> Gathers all the new info accumulated by the senses, since the last time GatherSenseInfo was called. </summary>
        protected virtual void SensoryInput()
        {
            if (newSenseInfo == null) { newSenseInfo = new List<Information>(); }
            else { newSenseInfo.Clear(); }

            if (sight != null) sight.NewInformation(newSenseInfo);
            if (hearing != null) hearing.NewInformation(newSenseInfo);
            if (touch != null) touch.NewInformation(newSenseInfo);
            if (interoception != null) interoception.NewInformation(newSenseInfo);
        }

        /// <summary>
        ///  Combines newSenseInfo into shortTermMemory and adds a latentPriorities entry for each new piece of information.
        ///   <para>Things in shortTermMemory that haven't changed will have their priorities lowered into the Unchanged set of options, if not there already.</para>
        ///   Things that start in both newSenseInfo AND shortTermMemory have their priorities moved from New to the Old Changed set of options and updated.
        ///   <para>Things not already in shortTermMemory are added under the New set of options.</para>
        /// </summary>
        protected virtual void Latency()
        {
            for (int j = 0; j < newSenseInfo.Count; j++)
            {
                for (int i = 0; i < shortTermMemory.Count; i++)
                {

                }
            }
        }

        /// <summary>
        /// Adjusts the excitation of each emotion based on the present state of the shortTermMemory.
        /// <para>When the state of an emotion changes, it can alter other emotions,
        /// mental states, the perceptions of the senses, the priorities of all the items in shortTermMemory, etc.</para>
        /// </summary>
        protected virtual void EmotionalResponse()
        {

        }

        /// <summary>
        /// Applies priority changes, if any mental states have been altered in any cognitive steps before now.
        /// </summary>
        protected virtual void AlterMentalState()
        {

        }

        /// <summary>
        /// Decides what to keep and process into the memory InformationMatrix, what to keep in shortTermMemory, and what to discard from shortTermMemory.
        /// </summary>
        protected virtual void ProcessToMemory()
        {

        }

        #endregion

        #region Senses

        public UnitySight sight;
        public UnityHearing hearing;
        public UnityTouch touch;
        public UnityInteroception interoception;

        protected virtual void SetupSenses()
        {
            sight.Setup(gameObject);

            hearing.Setup(gameObject);

            touch.Setup(gameObject);
        }

        public virtual void SetupAsGhost()
        {
            SetupSenses();
        }

        public virtual T GetSense<T>(int id) where T : ISense
        {
            switch (id)
            {
                case 0:
                    if(sight is T tSight) { return tSight; }
                    break;
                case 1:
                    if(touch is T tTouch) { return tTouch; }
                    break;
                case 2:
                    if(hearing is T tHearing) { return tHearing; }
                    break;
            }
            return default;
        }
        
        public virtual int GetSenseID<T>(T sense) where T : ISense
        {
            if (sense.Equals(sight)) { return 0; }
            if (sense.Equals(touch)) { return 1; }
            if (sense.Equals(hearing)) { return 2; }
            return -1;
        }

        #endregion

        #region Communications
        public Speaker voice;

        #endregion

        public UnitySentence[] ThoughtsToWords(int[] sememes)
        {
            return new UnitySentence[0];
        }

        public static UnityMind NewUnityMind(Transform parent = null)
        {
            GameObject brain = new GameObject("New Mind");
            if (parent != null)
            {
                brain.transform.SetParent(parent, false);
                brain.transform.localEulerAngles = brain.transform.localPosition = Vector3.zero;

            }

            UnityMind agent = AddUnityMind(brain);

            return agent;
        }

        public static UnityMind AddUnityMind(GameObject brain)
        {
            UnityMind mind = brain.AddComponent<UnityMind>();

            RaySensor eyes = brain.AddComponent<RaySensor>();
            eyes.IsSensing = true;

            TriggerSensor periphery = brain.AddComponent<TriggerSensor>();
            periphery.IsSensing = true;

            AudioSensor ears = brain.AddComponent<AudioSensor>();
            ears.IsSensing = true;

            mind.sight = new UnitySight() { mind = mind, linesOfSight = eyes, peripheralVision = periphery};
            mind.hearing = new UnityHearing() { mind = mind, hearing = ears };
            
            return mind;
        }
    }

    [Serializable]
    public class UnityMentalState : IMentalState
    {
        public Int excitationLevel;

        public T ExcitationLevel<T, TValue>() where T : IValue<TValue>
        {
            throw new System.NotImplementedException();
        }
    }

    [Serializable]
    public class UnityEmotion : UnityMentalState, IEmotion
    {
        public UnityFeeling feeling;

        public T Feeling<T>() where T : IFeeling
        {
            throw new System.NotImplementedException();
        }
    }

    [Serializable]
    public class UnityFeeling : IFeeling
    {
        public event Action OnFeel;
    }

    public enum StimulusPriority
    {
        OldNeutralUnchanged = 1,
        OldBeneficialUnchanged = 2,
        OldUnknownUnchanged = 3,

        OldNeutralChanged = 4,
        OldBeneficialChanged = 5,
        
        NewNeutral = 6,
        NewBeneficial = 7,

        OldThreatUnchanged = 8,
        OldUnknownChanged = 9,
        OldThreatChanged = 10,

        NewUnknown = 11,
        NewThreat = 12
    }
    
}