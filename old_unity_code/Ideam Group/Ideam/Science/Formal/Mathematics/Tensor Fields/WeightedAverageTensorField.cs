using System.Collections.Generic;

namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public class WeightedAverageTensorField
        : ITensorField
    {
        private float _totalWeight;
        private readonly List<KeyValuePair<ITensorField, float>> _blends = new List<KeyValuePair<ITensorField, float>>();

        public void Blend(ITensorField field, float weight = 1)
        {
            _blends.Add(new KeyValuePair<ITensorField, float>(field, weight));
            _totalWeight += weight;
        }


        public void Sample(ref Float2 position, out Tensor result)
        {
            result = new Tensor(0, 0);

            foreach (var b in _blends)
                result += (b.Value / _totalWeight) * b.Key.Sample(position);
        }

    }
}
