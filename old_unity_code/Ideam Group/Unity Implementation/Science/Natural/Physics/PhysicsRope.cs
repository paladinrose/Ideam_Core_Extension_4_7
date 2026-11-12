using System.Collections;
using System.Collections.Generic;
using UnityEngine;

using Ideam.Science.Formal.Mathematics.Geometry.Splines;

namespace Ideam.Unity.Science.Natural.Physics
{
    public class PhysicsRope : MonoBehaviour
    {
        public GameObject ropeSegment;
        public float segmentHeightAdjustment;
        public Rigidbody ropeBody;
        public Vector3 ropeAnchorPoint;

        public int numberOfPhysicsSolvers = 6;
        public int numberOfSegmentsPerSolver = 5;

        public Spline3 ropeSpline;

        public int totalSegments;

        public float segmentScalar;

        public Transform ropeTransform;

        public HingeJoint[] solvers;

        public Rigidbody[] solverBodies;

        public Transform[] segments;



        public bool RopeIsGenerated { get { return (totalSegments == numberOfPhysicsSolvers * numberOfSegmentsPerSolver && segments.Length == totalSegments); } }

        public Vector3[] SplinePoints
        {
            get { return ropeSpline.AnchorPoints.ToVector3(); }
            set { ropeSpline.SetAnchorPoints(value.ToFloat3()); }
        }

        [ContextMenu("Reconstruct this beyotch")]
        void ReconstructChain()
        {
            solvers = GetComponentsInChildren<HingeJoint>();
            int l = solvers.Length;

            solverBodies = new Rigidbody[l];
            ropeSpline = new Spline3(l, CurveType.CatmullRom);
            for (int i = 0; i < l; i++)
            {
                solverBodies[i] = solvers[i].GetComponent<Rigidbody>();
                ropeSpline[i] = transform.InverseTransformPoint(solvers[i].transform.position).ToFloat3();
            }

            List<Transform> segs = new List<Transform>();
            Transform[] allKids = GetComponentsInChildren<Transform>();
            for (int i = 0; i < allKids.Length; i++)
            {
                if (allKids[i].name.StartsWith("Rope Segment")) { segs.Add(allKids[i]); }
            }
        }

        private void Start()
        {
            if (!RopeIsGenerated) GenerateRope();
        }
        public void ResetAndGenerateRope()
        {
            ResetRope();
            GenerateRope();
        }

        public void ResetRope()
        {
            for (int i = 0; i < segments.Length; i++)
            {
#if UNITY_EDITOR
                DestroyImmediate(segments[i].gameObject);
#else
            Destroy(segments[i].gameObject);
#endif
            }
            segments = new Transform[0];
            for (int i = 0; i < solverBodies.Length; i++)
            {
#if UNITY_EDITOR
                DestroyImmediate(solverBodies[i].gameObject);
#else
            Destroy(solverBodies[i].gameObject);
#endif
            }
            solvers = new HingeJoint[0];
            solverBodies = new Rigidbody[0];
        }
        public void GenerateRope()
        {
            ropeTransform = ropeBody.transform;

            totalSegments = numberOfPhysicsSolvers * numberOfSegmentsPerSolver;
            segmentScalar = 1f / totalSegments;

            Bounds segmentBounds = ropeSegment.GetComponentInChildren<Renderer>().bounds;
            float segmentHeight = segmentBounds.size.z + segmentHeightAdjustment;
            float solverHeight = segmentHeight * numberOfSegmentsPerSolver;

            solvers = new HingeJoint[numberOfPhysicsSolvers];
            solverBodies = new Rigidbody[numberOfPhysicsSolvers];

            Vector3 solverColliderSize = new Vector3(segmentBounds.size.x, segmentBounds.size.y, solverHeight);
            Vector3 solverColliderCenter = new Vector3(0, 0, solverHeight * 0.5f);

            for (int i = 0; i < numberOfPhysicsSolvers; i++)
            {
                GameObject solverObject = new GameObject("Rope Solver " + (i + 1));

                solverObject.transform.SetParent(ropeTransform);
                solverObject.transform.localPosition = ropeAnchorPoint + new Vector3(0, 0, solverHeight * i);

                solverBodies[i] = solverObject.AddComponent<Rigidbody>();
                solvers[i] = solverObject.AddComponent<HingeJoint>();

                if (i == 0) { solvers[i].connectedBody = ropeBody; }
                else { solvers[i].connectedBody = solverBodies[i - 1]; }

                BoxCollider solverBox = solverObject.AddComponent<BoxCollider>();
                if (i != numberOfPhysicsSolvers - 1)
                {
                    solverBox.size = solverColliderSize;
                    solverBox.center = solverColliderCenter;
                }
                else
                {
                    solverBox.size = new Vector3(segmentBounds.size.x, segmentBounds.size.y, segmentBounds.size.z * 0.25f);
                    solverBox.center = new Vector3(segmentBounds.center.x, segmentBounds.center.y, segmentBounds.center.z * 0.25f);
                }
            }

            ropeSpline = new Spline3(numberOfPhysicsSolvers, CurveType.CatmullRom);
            UpdateSpline();

            segments = new Transform[totalSegments];

            for (int i = 0; i < totalSegments; i++)
            {
                GameObject segmentObject = Instantiate(ropeSegment);
                segmentObject.name = "Rope Segment " + (i + 1);
                segments[i] = segmentObject.transform;
                segments[i].SetParent(ropeTransform);
            }

            UpdateSegments();
        }

        public void PositionRopeAlongSpline()
        {
            //First, in the controls for this spline (in editor), when I move a point, I want it to be distance bound to the previous point
            //and all following points to move relativistically WITH the point I'm moving.
            //That way, when I call THIS function, all of the distances will already be accounted for.
            if (segments.Length != totalSegments)
            {
                Debug.LogError("This physics rope has not been generated correctly. Cannot position along spline.");
                return;
            }

            //A. Move all our solver bodies to the right locations and rotate them to look at the next point.
            for (int i = 0; i < solverBodies.Length - 1; i++)
            {
                Rigidbody sb = solverBodies[i];
                Vector3 lookDir = transform.InverseTransformDirection((ropeSpline[i + 1] - ropeSpline[i]).ToVector3());
                Vector3 forward = sb.transform.forward;
                Vector3 localTarget = sb.transform.InverseTransformPoint(transform.TransformPoint(ropeSpline[i + 1].ToVector3()));

                float angle = Mathf.Atan2(localTarget.x, localTarget.z) * Mathf.Rad2Deg;

                Vector3 eulerAngleVelocity = new Vector3(0, angle, 0);
                Quaternion deltaRotation = Quaternion.Euler(eulerAngleVelocity);
                if (Application.isEditor && !Application.isPlaying)
                {
                    sb.transform.localPosition = ropeSpline[i].ToVector3();
                    sb.transform.rotation *= deltaRotation;
                }
                else
                {
                    sb.position = transform.TransformPoint(ropeSpline[i].ToVector3());
                    sb.MoveRotation(sb.rotation * deltaRotation);
                }
            }
            int sbl = solverBodies.Length - 1;

            if (Application.isEditor && !Application.isPlaying)
            {
                solverBodies[sbl].transform.localPosition = ropeSpline[sbl].ToVector3();
                solverBodies[sbl].transform.rotation = solverBodies[sbl - 1].rotation;
            }
            else
            {
                solverBodies[sbl].position = transform.TransformPoint(ropeSpline[sbl].ToVector3());
                solverBodies[sbl].rotation = solverBodies[sbl - 1].rotation;
            }

            //B. Call UpdateSegments 
            UpdateSegments();

        }

        private void Update()
        {
            UpdateSpline();

            UpdateSegments();
        }

        void UpdateSpline()
        {
            for (int i = 0; i < numberOfPhysicsSolvers; i++)
            {
                ropeSpline[i] = solvers[i].transform.localPosition.ToFloat3();
            }
        }

        void UpdateSegments()
        {
            segments[0].localPosition = ropeSpline[0].ToVector3();

            for (int i = 1; i < totalSegments; i++)
            {
                segments[i].localPosition = ropeSpline.GetPoint(i * segmentScalar).ToVector3();
                segments[i - 1].LookAt(segments[i]);
            }

            segments[totalSegments - 1].eulerAngles = segments[totalSegments - 2].eulerAngles;
        }
    }
}