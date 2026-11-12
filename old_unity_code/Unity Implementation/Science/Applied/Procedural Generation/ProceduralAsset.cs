using System;
using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.AddressableAssets;


namespace Ideam.Unity.Science.Applied.ProceduralGeneration
{
   
    public class ProceduralAsset : ScriptableObject
    {

        protected bool assetIsDirty;

        #region ScriptableObject Implementation

        /// <summary>
        /// Awake is called as the Procedural Asset script starts. This happens as the game is launched and is similar to MonoBehavior.Awake.
        /// </summary>
        protected virtual void Awake()
        {

        }

        /// <summary>
        /// This function is called when the Procedural Asset is loaded.
        /// </summary>
        protected virtual void OnEnable()
        {
            
        }

        /// <summary>
        /// This function is called when the Procedural Asset goes out of scope.
        /// </summary>
        protected virtual void OnDisable() 
        {
        }

        /// <summary>
        /// This function is called when the Procedural Asset will be destroyed.
        /// </summary>
        protected virtual void OnDestroy()
        {
            
        }

        #if UNITY_EDITOR

        /// <summary>
        /// Editor-only function that Unity calls when the script is loaded or a value changes in the Inspector.
        /// </summary>
        protected virtual void OnValidate()
        {
            
        }

        /// <summary>
        /// Reset is called when the user hits the Reset button in the Inspector's context menu or when adding the Procedural Asset the first time. 
        /// <para> This function is only called in editor mode. </para> 
        /// <para> Reset is most commonly used to give good default values in the Inspector. </para>
        /// </summary>
        protected virtual void Reset() 
        {

        }
#endif

        #endregion

        #region Asset References

        public virtual bool AssetReferencesReady { get; protected set; }

        public AssetReference[] assetReferences;

        protected virtual void ValidateAssetReferences()
        {
            bool ready = true;
            for(int i =0; i < assetReferences.Length; i++)
            {
                var a = assetReferences[i];
                if(!a.IsValid() || !a.IsDone) { ready = false; break; }
            }
            AssetReferencesReady = ready;
        }

        #endregion

        #region Procedural Generation

        public virtual bool IsGenerated { get; protected set; }
        public virtual bool HoldProceduralGenerationForAssetReferences { get; protected set; }

        public virtual event Action OnBeginProceduralGeneration, 
                                    OnUpdateProceduralGeneration, 
                                    OnCompleteProceduralGeneration, 
                                    OnFailProceduralGeneration;

        public virtual IEnumerator ProceduralGeneration() 
        {
            Debug.Log(Time.realtimeSinceStartup + " | Beginning Procedural Generation.");

            OnBeginProceduralGeneration?.Invoke();

            yield return null;

            Debug.Log(Time.realtimeSinceStartup + " | " + "Updating Procedural Generation.");

            OnUpdateProceduralGeneration?.Invoke();

            yield return null;

            Debug.Log(Time.realtimeSinceStartup + " | " + "Completing Procedural Generation.");

            OnCompleteProceduralGeneration?.Invoke();

            yield break; 
        }

        #endregion
    }


}