
using UnityEngine;

namespace Ideam.Unity.Axiology.Aesthetics.The_Arts.Animation
{
    public class AnimationControl : MonoBehaviour
    {
        public RelativeObjectShadow lockSettings;

        public AnimationPose[] poses = new AnimationPose[0];

        Mesh cubeMesh;
       
        public void UpdateControl()
        {
            lockSettings.ShadowObject();
            
        }

        public bool Locked { get { return lockSettings.targetObject; } }

        public void Lock(Transform lockTo, bool setOffsets = false)
        {
            if (setOffsets)
            {
                if (lockSettings.shadowPosition)
                {
                    lockSettings.positionOffset = lockTo.position - transform.position;
                }
                if (lockSettings.shadowRotation)
                {
                    lockSettings.rotationOffset = Quaternion.FromToRotation(transform.eulerAngles, lockTo.eulerAngles).eulerAngles;
                }
            }
            lockSettings.targetObject = lockTo;
        }


        public void ReleaseLock()
        {
            lockSettings.targetObject = null;
        }

        public void Pose(int poseID, float t)
        {
            if (poseID < 0 || poseID >= poses.Length) { return; }

            if (lockSettings.shadowRotation) transform.localEulerAngles = poses[poseID].rotationSpline.GetPoint(t).ToVector3();

            if (lockSettings.shadowPosition) transform.localPosition = poses[poseID].positionSpline.GetPoint(t).ToVector3();
        }

        public void BlendPoses(int poseA, float tA, int poseB, float tB, float blend)
        {
            if (poseA < 0 || poseB < 0 || poseA >= poses.Length || poseB >= poses.Length) { return; }

            if (lockSettings.shadowRotation)
            {
                Vector3 rotA = poses[poseA].rotationSpline.GetPoint(tA).ToVector3();
                Vector3 rotB = poses[poseB].rotationSpline.GetPoint(tB).ToVector3();
                transform.localEulerAngles = new Vector3(
                    Mathf.LerpAngle(rotA.x, rotB.x, blend),
                    Mathf.LerpAngle(rotA.y, rotB.y, blend),
                    Mathf.LerpAngle(rotA.z, rotB.z, blend));
            }

            if (lockSettings.shadowPosition)
            {
                Vector3 posA = poses[poseA].positionSpline.GetPoint(tA).ToVector3();
                Vector3 posB = poses[poseB].positionSpline.GetPoint(tB).ToVector3();
                transform.localPosition = Vector3.Lerp(posA, posB, blend);
            }
        }

        private void OnDrawGizmosSelected()
        {
            if(!lockSettings || !lockSettings.targetObject) { return; }

            Gizmos.color = Color.yellow;

            Vector3 position = lockSettings.targetObject.position;
            position.Scale(lockSettings.positionScaling);
            position += lockSettings.positionOffset;

            Vector3 rotation = lockSettings.targetObject.eulerAngles;
            rotation.Scale(lockSettings.rotationScaling);

            Quaternion rot = Quaternion.Euler(rotation);

            rot *= Quaternion.Euler(lockSettings.rotationOffset);

            if (!cubeMesh) { cubeMesh = Resources.GetBuiltinResource<Mesh>("Cube.fbx"); }
            Gizmos.DrawWireMesh(cubeMesh, position, rot, Vector3.one*0.01f);
        }
    }
}