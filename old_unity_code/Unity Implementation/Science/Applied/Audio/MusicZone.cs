using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Ideam.Unity.Metaphysics.Mind.Perception.Sensors;
using System;

namespace Ideam.Unity.Science.Applied.Audio
{
    public class MusicZone : MonoBehaviour
    {
        public Jukebox jukebox;

        [SerializeField]
        private Jukebox globalJukebox;
        public Jukebox GlobalJukebox
        {
            get { return globalJukebox; }
            set
            {
                if (value != globalJukebox)
                {
                    if (value == null)
                    {

                    }
                    else
                    {
                        globalJukebox = value;
                        globalLoop = value.loopMode;
                        globalPlaylist = value.PlaylistID();
                    }
                }
            }
        }

        public int[] trackList;
        public bool randomizeTracks;
        public Jukebox.LoopMode loopMode = Jukebox.LoopMode.All;

        public TriggerSensor zoneTrigger;

        Jukebox.LoopMode globalLoop;
        int globalPlaylist;

        private void Start()
        {
            zoneTrigger.OnTriggerStart.hitEvent += PlayZoneMusic;
            zoneTrigger.OnTriggerStop.hitEvent += StopZoneMusic;
        }

        private void StopZoneMusic(GameObject obj)
        {
            if (jukebox)
            {
                jukebox.StopPlaylist();
                globalJukebox.PlayList(trackList, randomizeTracks);

            }
            else
            {
                globalJukebox.loopMode = globalLoop;

            }
        }

        void PlayZoneMusic(GameObject zoneEnterer)
        {
            if (jukebox)
            {
                globalJukebox.StopPlaylist();

                jukebox.loopMode = loopMode;
                jukebox.PlayList(trackList, randomizeTracks);

            }
            else
            {
                globalJukebox.loopMode = loopMode;
                globalJukebox.PlayList(trackList, randomizeTracks);
            }
        }
    }
}