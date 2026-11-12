using System;
using UnityEngine;
using UnityEngine.Events;
using System.Collections;
using System.Collections.Generic;
#if UNITY_EDITOR
using UnityEditor;
#endif

namespace Ideam.Unity.Metaphysics.Mind.Perception.Sensors
{
    [AddComponentMenu("Ideam/Sensors/Audio Sensor")]
    public class AudioSensor : BaseSensor
    {
        public AudioSensorEvent OnStartHear, OnHear, OnStopHear;
        
        public float onHearTimer;

        public List<DetectableAudio> detectableAudio = new List<DetectableAudio>();
        public override bool IsSensing
        {
            get { return isSensing; }
            set
            {
                if (isSensing != value)
                {
                    if (isSensing)
                    {
                        //tell all detectableaudios that we're no longer sensing.
                    }
                    else
                    {
                        //tell all detectableaudios that we're sensing.
                    }
                    base.IsSensing = value;
                }

            }
        }

    }

    [System.Serializable]
    public struct AudioSensorData
    { 
        public DetectableAudio detectedAudio;
        public int soundID;
        public AudioSource source;
        public float duration;

        public AudioSensorData(DetectableAudio s, int id)
        {
            detectedAudio = s;
            soundID = id;
            source = detectedAudio.soundSources[soundID].source;
            duration = 0;
        }
    }

    public delegate void AudioSensorDataHandler(AudioSensorData data);

    [System.Serializable]
    public class AudioSensorEvent : SensorEvent<AudioSensorData>
    {
        public event AudioSensorDataHandler sensorEvent;

        public override bool Invoke(GameObject targetObject, AudioSensorData eventData)
        {
            if(base.Invoke(targetObject, eventData))
            {
                sensorEvent?.Invoke(eventData);
                return true;
            }
            return false;
        }
    }
}