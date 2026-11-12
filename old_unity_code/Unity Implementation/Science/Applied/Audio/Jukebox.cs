using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Ideam.Unity.Science.Applied.Audio
{
    public class Jukebox : MonoBehaviour
    {

        public enum LoopMode { None, Track, All };
        public enum Startup { None, All, RandomAll };

        public List<AudioClip> tracks = new List<AudioClip>();

        public float crossfadeTime;

        public LoopMode loopMode;
        public Startup startup = Startup.RandomAll;

        public AudioSource sourceA, sourceB;
        public GameObject sourceObjectA, sourceObjectB;

        public List<int[]> builtinPlaylists = new List<int[]>();


        AudioSource currentSource;

        int[] currentPlaylist, swapToList;

        int builtinID = -1, currentSong, swapToSong;

        Coroutine playRoutine, fadeOutRoutine, fadeInRoutine;

        bool paused, randomizeOnLoop, usingSourceA;

        public int PlaylistID()
        {
            return builtinID;
        }

        private void Awake()
        {

            switch (startup)
            {
                case Startup.All:
                    PlayAll(false);
                    break;
                case Startup.RandomAll:
                    PlayAll(true);
                    break;
            }
        }

        public void PlayAll(bool randomize)
        {
            if (playRoutine != null) { StopPlaylist(); }

            currentPlaylist = new int[tracks.Count];
            currentSong = 0;
            if (!randomize)
            {
                for (int i = 0; i < tracks.Count; i++)
                {
                    currentPlaylist[i] = i;
                }
                randomizeOnLoop = false;
            }
            else
            {
                List<int> usedIDs = new List<int>();
                usedIDs.Add(tracks.Count - 1);
                for (int i = 0; i < tracks.Count - 1; i++)
                {
                    int newID = UnityEngine.Random.Range(0, usedIDs.Count);
                    usedIDs.Insert(newID, i);
                }
                currentPlaylist = usedIDs.ToArray();
                randomizeOnLoop = true;
            }
            StartPlaylist();
        }

        public void PlayList(int[] playlist, bool randomize)
        {
            if (playRoutine != null) { StopPlaylist(); }
            currentPlaylist = playlist;
            currentSong = 0;
            if (randomize)
            {
                List<int> newPlaylist = new List<int>();
                newPlaylist.Add(currentPlaylist[0]);
                for (int i = currentPlaylist.Length - 1; i >= 1; i--)
                {
                    int newID = UnityEngine.Random.Range(0, newPlaylist.Count);
                    newPlaylist.Insert(newID, currentPlaylist[i]);
                }
                currentPlaylist = newPlaylist.ToArray();
                randomizeOnLoop = true;
            }
            else { randomizeOnLoop = false; }
            StartPlaylist();
        }

        void StartPlaylist()
        {
            playRoutine = StartCoroutine(PlayRoutine());
        }

        public void StopPlaylist()
        {
            sourceA.Stop();
            sourceB.Stop();

            StopCoroutine(playRoutine);
        }

        public void Pause()
        {
            if (!paused)
            {
                sourceA.Pause();
                sourceB.Pause();
                paused = true;
            }
            else
            {
                sourceA.Play();
                sourceB.Play();
                paused = false;
            }
        }

        IEnumerator PlayRoutine()
        {
            float fadeScalar = 0f;
            if (crossfadeTime > 0) fadeScalar = 1f / crossfadeTime;

            if (usingSourceA) { currentSource = sourceB; } else { currentSource = sourceA; }
            while (currentSong < currentPlaylist.Length)
            {
                currentSource.clip = tracks[currentPlaylist[currentSong]];
                currentSource.Play();

                yield return null;

                while (currentSource.isPlaying || paused)
                {
                    yield return null;
                }

                switch (loopMode)
                {
                    case LoopMode.None:
                        currentSong++;
                        break;

                    case LoopMode.All:
                        currentSong++;
                        if (currentSong >= currentPlaylist.Length)
                        {
                            currentSong = 0;
                            if (randomizeOnLoop)
                            {
                                List<int> newPlaylist = new List<int>();
                                newPlaylist.Add(currentPlaylist[0]);
                                for (int i = currentPlaylist.Length - 1; i >= 1; i--)
                                {
                                    int newID = UnityEngine.Random.Range(0, newPlaylist.Count);
                                    newPlaylist.Insert(newID, currentPlaylist[i]);
                                }
                                currentPlaylist = newPlaylist.ToArray();
                            }
                        }
                        break;
                }
            }
        }

        //gradually shifts the sound comming from our audio sources to the this clip:
        // maxVolume should be in 0-to-1 range
        public void CrossFade(AudioClip clipToPlay, float maxVolume, float fadingTime, float delay_before_crossFade = 0)
        {
            //var fadeRoutine = StartCoroutine(Fade(clipToPlay, maxVolume, fadingTime, delay_before_crossFade));
            StartCoroutine(Fade(clipToPlay, maxVolume, fadingTime, delay_before_crossFade));

        }//end CrossFade()


        IEnumerator Fade(AudioClip playMe, float maxVolume, float fadingTime, float delay_before_crossFade = 0)
        {
            if (delay_before_crossFade > 0)
            {
                yield return new WaitForSeconds(delay_before_crossFade);
            }

            AudioSource curActiveSource, newActiveSource;
            if (usingSourceA)
            {
                //_source0 is currently playing the most recent AudioClip
                curActiveSource = sourceA;
                //so launch on _source1
                newActiveSource = sourceB;
            }
            else
            {
                //otherwise, _source1 is currently active
                curActiveSource = sourceB;
                //so play on _source0
                newActiveSource = sourceA;
            }

            //perform the switching
            newActiveSource.clip = playMe;
            newActiveSource.Play();
            newActiveSource.volume = 0;

            if (fadeOutRoutine != null)
            {
                StopCoroutine(fadeOutRoutine);
            }

            if (fadeInRoutine != null)
            {
                StopCoroutine(fadeInRoutine);
            }

            fadeOutRoutine = StartCoroutine(FadeAudioSource(curActiveSource, curActiveSource.volume, 0, fadingTime));
            fadeInRoutine = StartCoroutine(FadeAudioSource(newActiveSource, newActiveSource.volume, maxVolume, fadingTime));

            usingSourceA = !usingSourceA;

            yield break;
        }


        IEnumerator FadeAudioSource(AudioSource sourceToFade, float startVolume, float endVolume, float duration)
        {
            float startTime = Time.time;

            while (true)
            {
                float elapsed = Time.time - startTime;

                sourceToFade.volume = Mathf.Clamp01(Mathf.Lerp(startVolume, endVolume, elapsed / duration));

                if (sourceToFade.volume == endVolume)
                {
                    break;
                }

                yield return null;
            }//end while
        }


        //returns false if BOTH sources are not playing and there are no sounds are staged to be played.
        //also returns false if one of the sources is not yet initialized
        public bool IsPlaying
        {
            get
            {
                if (sourceA == null || sourceB == null)
                {
                    return false;
                }

                //otherwise, both sources are initialized. See if any is playing:
                return sourceA.isPlaying || sourceB.isPlaying;
            }
        }
    }
}