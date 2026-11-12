using System.Collections;
using System.Collections.Generic;

using UnityEngine;
using UnityEngine.Animations.Rigging;

using Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Ludology
{
    //Later on, this needs to becomes a "HandedAgent", and "Humanoid" can descend from that. For now,
    //it'll do.
    public class TwoHandedAgent : UnityGameAgent
    {
        public float eyelineHeight;
        public RigBuilder rigBuilder;

        public HandAnimationRigging handsRig;
        
        #region Gesture Management Variables

        [Tooltip("If the Trigger control is held near the halfway point for this length of time, the" +
            "half-index curl gesture will engage. If less than or equal to 0, half-index gesture is not used.")]
        public float halfIndexGestureTime;

        [Tooltip("The action associated with the half-index finger curl gesture. Not used if less than 0.")]
        public int halfIndexActionID;

        [Tooltip("The action associated with the full index finger curl gesture. Not used if less than 0.")]
        public int indexActionID;

        [Tooltip("If the Grip control is held near the halfway point for this length of time, the" +
            "half-grip gesture will engage. If less than or equal to 0, half-grip gesture is not used.")]
        public float halfGripGestureTime;

        [Tooltip("The action associated with the half-grip hand gesture. Not used if less than 0.")]
        public int halfGripActionID;

        [Tooltip("The action associated with the full-grip hand gesture. Not used if less than 0.")]
        public int gripActionID;

        [Tooltip("If true, the fist gesture and half-fist gesture will be used in place of " +
            "the grip or index finger gestures. Their values will be averaged for the fist gesture value.")]
        public bool averageGripAndIndexCurlAsFistValue;

        [Tooltip("The action associated with the half-fist hand gesture. Not used if less than 0.")]
        public int halfFistActionID;

        
        [Tooltip("The action associated with the full-fist hand gesture. Not used if less than 0.")]
        public int fistActionID;

        
        protected float currentLeftGrip, currentLeftHalfGripTimer, currentRightGrip, currentRightHalfGripTimer,
            currentLeftIndex, currentRightIndex,
            currentLeftFist, currentRightFist;
        
        protected bool holdingLeft, holdingRight;

        public virtual bool HoldingLeft { get { return holdingLeft; } }
        public virtual bool HoldingRight { get { return holdingRight; } }
        
        #endregion

        #region Gesture Management

        /// <summary>
        /// Curls all fingers. Forms a C shape at 0.25. If held for a brief moment at 0.5, index
        /// finger and thumb will form an O shape. If the O shape is not engaged, full fist is at 1.
        /// If O shape is engaged, then value rises to 1, hand will take a cats-paw gesture with
        /// thumb free, and fingers straightened at the first going, and fully curled at all other joints.
        /// </summary>
        /// <param name="fistValue">The degree that the fist gesture is engaged</param>
        public virtual void LeftFist(float fistValue) 
        {
            currentLeftFist = fistValue;
            //handsRig.riggedHands[0].CurlFist(fistValue);
        }

        /// <summary>
        /// Curls all fingers but the index finger. If held for a brief moment at 0.5, middle 
        /// finger and thumb will form an O, leaving the index finger extended. If fully engaged
        /// without forming this gesture, the hand will close, leaving the index finger to point.
        /// If fully engaged after forming the O gesture, the hand will make a 'finger gun' gesture.
        /// </summary>
        /// <param name="gripValue"></param>
        public virtual void LeftGrip(float gripValue) 
        {
            currentLeftGrip = gripValue;
            //handsRig.riggedHands[0].Gesture(
            //                  handsRig.riggedHands[0].pointGesture, gripValue);
        }

        public virtual void LeftGripStart() 
        {
        }

        public virtual void LeftGripEnd() 
        {
            currentLeftGrip = currentLeftFist = 0;
            //handsRig.riggedHands[0].Gesture(0, 1f);
        }
        /// <summary>
        /// Curls just the index finger, by itself. If held briefly at 0.5, thumb and
        /// finger will make the "very small" gesture, slowly approaching one another, but stopping
        /// with a small gap left. If fully engaged without the gesture, finger will curl all the way, 
        /// leaving thumb free. If fully engaged after the gesture, fingre and thumb will pinch. 
        /// </summary>
        /// <param name="curlValue"></param>
        public virtual void LeftIndexCurl(float curlValue) 
        {
            currentLeftIndex = curlValue; 
            if(currentLeftGrip > 0) { return; }

            //handsRig.riggedHands[0].Gesture( handsRig.riggedHands[0].pinchGesture, curlValue);
        }
        public virtual void LeftIndexCurlStart() { }
        public virtual void LeftIndexCurlEnd() 
        {
            currentLeftIndex = 0;
            //handsRig.riggedHands[0].Gesture(0, 1f);
        }

        /// <summary>
        /// Curls all fingers. Forms a C shape at 0.25. If held for a brief moment at 0.5, index
        /// finger and thumb will form an O shape. If the O shape is not engaged, full fist is at 1.
        /// If O shape is engaged, then value rises to 1, hand will take a cats-paw gesture with
        /// thumb free, and fingers straightened at the first going, and fully curled at all other joints.
        /// </summary>
        /// <param name="fistValue">The degree that the fist gesture is engaged</param>
        public virtual void RightFist(float fistValue)
        {
            currentRightFist = fistValue;
            //handsRig.riggedHands[1].CurlFist(fistValue);
        }

        /// <summary>
        /// Curls all fingers but the index finger. If held for a brief moment at 0.5, middle 
        /// finger and thumb will form an O, leaving the index finger extended. If fully engaged
        /// without forming this gesture, the hand will close, leaving the index finger to point.
        /// If fully engaged after forming the O gesture, the hand will make a 'finger gun' gesture.
        /// </summary>
        /// <param name="gripValue"></param>
        public virtual void RightGrip(float gripValue)
        {
            currentRightGrip = gripValue;
            //handsRig.riggedHands[1].Gesture(
            //                    handsRig.riggedHands[1].pointGesture, gripValue);
        }

        public virtual void RightGripStart()
        {
        }

        public virtual void RightGripEnd()
        {
            currentRightGrip = currentRightFist = 0;
            //handsRig.riggedHands[1].Gesture(0, 1f);
        }
        /// <summary>
        /// Curls just the index finger, by itself. If held briefly at 0.5, thumb and
        /// finger will make the "very small" gesture, slowly approaching one another, but stopping
        /// with a small gap left. If fully engaged without the gesture, finger will curl all the way, 
        /// leaving thumb free. If fully engaged after the gesture, fingre and thumb will pinch. 
        /// </summary>
        /// <param name="curlValue"></param>
        public virtual void RightIndexCurl(float curlValue)
        {
            currentRightIndex = curlValue;
            if (currentRightGrip > 0) { return; }

            //handsRig.riggedHands[1].Gesture( handsRig.riggedHands[1].pinchGesture, curlValue);
        }
        public virtual void RightIndexCurlStart() { }
        public virtual void RightIndexCurlEnd()
        {
            currentRightIndex = 0;
            //handsRig.riggedHands[1].Gesture(0,1f);
        }

        #endregion


        public virtual void LeftPrimary() { }

        public virtual void LeftSecondary() { }

        public virtual void LeftMenu() { }


        public virtual void RightPrimary() { }

        public virtual void RightSecondary() { }

        public virtual void RightMenu() { }

        protected override void OnDrawGizmos()
        {
            base.OnDrawGizmos();
            Gizmos.color = Color.black;
            Vector3 startPoint = transform.up * eyelineHeight;
            Vector3 endPoint = startPoint + (transform.forward * 2);
            Gizmos.DrawLine(startPoint, endPoint);
        }
    }
}