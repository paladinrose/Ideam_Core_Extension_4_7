using System.Collections.Generic;

namespace Ideam.Science.Formal.Mathematics.Tensors
{
    public class AdditiveTensorField : ITensorField
    {
        private readonly List<ITensorField> _fields = new List<ITensorField>();

        public AdditiveTensorField(params ITensorField[] fields)
        {
            foreach (var tensorField in fields)
                Add(tensorField);
        }

        public void Add(ITensorField field)
        {
            _fields.Add(field);
        }

        public void Sample(ref Float2 position, out Tensor result)
        {
            result = new Tensor(0, 0);

            foreach (var b in _fields)
                result += b.Sample(position);
        }

    }
}
