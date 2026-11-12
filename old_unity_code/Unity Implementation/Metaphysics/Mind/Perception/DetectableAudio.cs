using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics;
using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using Ideam.Unity.Science.Social.Communications;

namespace Ideam.Unity.Metaphysics.Mind.Perception
{
    public class DetectableAudio : UnityTransmitter<UnityAudioSignal>
    {
        public Transform audioOrigin;
        public SoundSource[] soundSources = new SoundSource[0];
        bool soundsArePlaying;

        public List<ListenerContainer> listeners = new List<ListenerContainer>();
        List<int> toRemove = new List<int>();
        public bool listening;


        public override event Action<UnityAudioSignal> OnBeginTransmitting;
        public override event Action<UnityAudioSignal> WhileTransmitting;
        public override event Action<UnityAudioSignal> OnStopTransmitting;

        public override void BeginTransmission(UnityAudioSignal signal)
        {
            if (currentSignals.Contains(signal)) { return; }

            currentSignals.Add(signal);

            OnBeginTransmitting?.Invoke(signal);
        }

        public override void EndTransmission(UnityAudioSignal signal)
        {
            if (!currentSignals.Contains(signal)) { return; }

            currentSignals.Remove(signal);

            OnStopTransmitting?.Invoke(signal);
        }

        public override UnityAudioSignal Transmit(UnityAudioSignal signal, Float deltaTime)
        {

            WhileTransmitting?.Invoke(signal);
            return signal;

        }

        public void PlaySound(int id)
        {
            if (enabled && id >= 0 && id < soundSources.Length)
            {
                soundSources[id].OnPlay.Invoke(soundSources[id]);
                soundSources[id].source.Play();
                if (!soundsArePlaying)
                {
                    StartCoroutine(UpdateDetectableAudio());
                }
            }
        }

        public void StopSound(int id)
        {
            if (id >= 0 && id < soundSources.Length)
            {
                if (soundSources[id].source.isPlaying)
                {
                    soundSources[id].source.Stop();
                    soundSources[id].OnStop.Invoke(soundSources[id]);
                }
            }
        }

        public SoundSource GetSound(string title)
        {
            for (int i = soundSources.Length - 1; i >= 0; i--)
            {
                if (soundSources[i].name == title) { return soundSources[i]; }
            }
            return null;
        }

        public void SetSoundSource(string title, AudioSource source)
        {
            for (int i = soundSources.Length - 1; i >= 0; i--)
            {
                if (soundSources[i].name == title) { soundSources[i].source = source; return; }
            }
        }

        void OnTriggerEnter(Collider col)
        {

            AudioSensor aS = col.GetComponent<AudioSensor>();
            if (aS != null)
            {
                bool isNew = true;
                for (int i = listeners.Count - 1; i >= 0; i--)
                {
                    if (aS == listeners[i].listener)
                    {
                        isNew = false;
                        break;
                    }
                }
                    
                if (isNew)
                {
                    ListenerContainer newListener = new ListenerContainer(aS, soundSources.Length);
                    listeners.Add(newListener);
                    aS.detectableAudio.Add(this);
                    if (listeners.Count == 1)
                    {
                        StartCoroutine(UpdateDetectableAudio());
                    }
                }
            }
        }

        void OnTriggerExit(Collider col)
        {
            AudioSensor aS = col.GetComponent<AudioSensor>();
            if (aS != null)
            {
                for (int i = listeners.Count - 1; i >= 0; i--)
                {
                    if (aS == listeners[i].listener)
                    {
                        toRemove.Add(i);
                    }
                }
            }
        }

        IEnumerator UpdateDetectableAudio()
        {
            while (listeners.Count > 0 || soundsArePlaying)
            {
                bool rem = false, refreshToRemoveList = false;
                for (int j = listeners.Count - 1; j >= 0; j--)
                {
                    if (rem)
                    {
                        listeners.RemoveAt(j + 1);
                        refreshToRemoveList = true;
                    }
                    rem = false;
                    soundsArePlaying = false;
                    if (listeners[j].listener.IsSensing)
                    {
                        for (int i = soundSources.Length - 1; i >= 0; i--)
                        {
                            if (soundSources[i].source.isPlaying == false && listeners[j].canHear[i] == false)
                            {
                                //I believe this break to be in error. Shoud be continue instead?
                                //break;
                                continue;
                            }
                            else
                            {
                                if (soundSources[i].source.isPlaying)
                                {
                                    soundsArePlaying = true;
                                    float soundDist = Vector3.Distance(listeners[j].listener.transform.position, soundSources[i].source.transform.position);
                                    if (soundDist <= soundSources[i].source.maxDistance)
                                    {
                                        if (!listeners[j].canHear[i])
                                        {
                                            listeners[j].currentUpdateTimers[i] = 0;
                                            listeners[j].listener.OnStartHear.Invoke(gameObject, new AudioSensorData(this, i));
                                            listeners[j].canHear[i] = true;
                                        }
                                        else
                                        {
                                            listeners[j].currentUpdateTimers[i] += Time.deltaTime;
                                            if (listeners[j].currentUpdateTimers[i] >= listeners[j].listener.onHearTimer)
                                            {
                                                listeners[j].currentUpdateTimers[i] -= listeners[j].listener.onHearTimer;
                                                listeners[j].listener.OnHear.Invoke(gameObject, new AudioSensorData(this, i));
                                            }
                                        }
                                    }
                                    else if (listeners[j].canHear[i])
                                    {
                                        listeners[j].listener.OnStopHear.Invoke(gameObject, new AudioSensorData(this, i));
                                        listeners[j].canHear[i] = false;
                                    }
                                }
                                else if (listeners[j].canHear[i])
                                {
                                    listeners[j].listener.OnStopHear.Invoke(gameObject, new AudioSensorData(this, i));
                                    listeners[j].canHear[i] = false;
                                }
                            }
                        }
                    }

                    for (int i = toRemove.Count - 1; i >= 0; i--)
                    {
                        if (toRemove[i] == j)
                        {
                            rem = true;
                        }
                    }
                }
                if (rem)
                {
                    listeners.RemoveAt(0);
                    refreshToRemoveList = true;
                }

                if(refreshToRemoveList) toRemove = new List<int>();

                yield return null;
            }
        }

        private void OnEnable()
        {
            
        }
        private void OnDisable()
        {
            
        }
    }

    [System.Serializable]
    public class SoundSource
    {
        public AudioSource source;
        public string name;
        public Action<SoundSource> OnPlay, OnPlaying, OnLoop, OnStop;
    }

    [System.Serializable]
    public class ListenerContainer
    {
        public AudioSensor listener;
        public bool[] canHear;
        public float[] currentUpdateTimers;

        public ListenerContainer(AudioSensor l, int numberOfSounds)
        {
            listener = l;
            canHear = new bool[numberOfSounds];
            currentUpdateTimers = new float[numberOfSounds];
        }
    }
}