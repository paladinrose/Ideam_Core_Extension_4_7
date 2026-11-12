using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Ideam.Science.Applied.Layout
{
    public interface ILayout
    {
        void SetupLayout<T>(T[] layoutElements);

        void PerformLayout();
    }
}