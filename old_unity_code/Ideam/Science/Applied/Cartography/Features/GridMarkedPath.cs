using System;

namespace Ideam.Science.Applied.Cartography.Features
{
    public class GridMarkedPath : AbstractSpline2Feature
    {
        public int[] gridFootprint = new int[0];
        public override Type Represents()
        {
            throw new NotImplementedException();
        }

        public override T Subject<T>()
        {
            throw new NotImplementedException();
        }
    }
}