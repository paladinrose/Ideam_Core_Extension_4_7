using System;
using System.Collections.Generic;

using Ideam.Science.Formal.Mathematics.Scalars;

namespace Ideam.Science.Formal.Mathematics.Tensors.Tracing
{
    public class TracingConfiguration
    {
        private readonly BaseScalarField _priorityField;
        /// <summary>
        /// Field which specifies the priority of a given point as a seed. Recommended that this contains normalized distance from nearest coastline
        /// </summary>
        public BaseScalarField PriorityField
        {
            get
            {
                return _priorityField;
            }
        }

        private readonly BaseScalarField _separationField;
        /// <summary>
        /// Separation between roads
        /// </summary>
        public BaseScalarField SeparationField
        {
            get
            {
                return _separationField;
            }
        }

        private readonly ITensorField _tensorField;
        /// <summary>
        /// Eigen vectors to trace streamlines along
        /// </summary>
        public ITensorField TensorField
        {
            get
            {
                return _tensorField;
            }
        }

        private readonly float _consineSearchConeAngle;
        internal float CosineSearchConeAngle
        {
            get { return _consineSearchConeAngle; }
        }

        public float SearchConeAngle
        {
            get
            {
                return (float)Math.Acos(_consineSearchConeAngle);
            }
        }

        private readonly float _segmentLength;
        /// <summary>
        /// The approximate length of a segment of road
        /// </summary>
        public float SegmentLength
        {
            get
            {
                return _segmentLength;
            }
        }

        private readonly float _mergeDistance;
        public float MergeDistance
        {
            get { return _mergeDistance; }
        }

        private readonly IValueGenerator _roadWidth;
        public IValueGenerator RoadWidth
        {
            get
            {
                return _roadWidth;
            }
        }

        public TracingConfiguration(BaseScalarField priorityField, BaseScalarField separationField, ITensorField tensorField, IValueGenerator roadWidth,
            float searchAngle = 0.3926991f, //22.5 degrees in radians
            float segmentLength = 10,
            float mergeDistance = 25
        )
        {

            _priorityField = priorityField;
            _separationField = separationField;
            _tensorField = tensorField;
            _roadWidth = roadWidth;
            _consineSearchConeAngle = (float)Math.Cos(searchAngle);
            _segmentLength = segmentLength;
            _mergeDistance = mergeDistance;
        }


        
    }

}