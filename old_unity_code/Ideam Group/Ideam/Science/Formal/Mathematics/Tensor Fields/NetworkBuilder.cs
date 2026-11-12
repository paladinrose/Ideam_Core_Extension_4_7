
using System;
using System.Collections;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;

using Ideam.Science.Formal.Mathematics.Geometry;
using Ideam.Science.Formal.Mathematics.VectorFields;
using Ideam.Science.Formal.Mathematics.Scalars;


namespace Ideam.Science.Formal.Mathematics.Tensors.Tracing
{
    public interface IBox
    {
        /// <summary>
        /// The value of this box
        /// </summary>
        object? Value { get; set; }

        /// <summary>
        /// The type of the value of this box
        /// </summary>
        Type Type { get; }
    }

    /// <summary>
    /// An object which contains a value
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public abstract class BaseBox<T>
        : IBox
    {
        /// <summary>
        /// The value this box contains.
        /// </summary>
        public abstract T? Value { get; set; }

        /// <summary>
        /// Gets or sets the value this box contains.
        /// </summary>
        /// <value>The value this box contains.</value>
        object? IBox.Value
        {
            get => Value;
            set
            {
                var old = Value;

                if (value == null)
                    Value = default;
                else
                    Value = (T)value;

                BoxChanged?.Invoke(this, old, Value);
            }
        }

        /// <summary>
        /// The type of the value in this box
        /// </summary>
        public Type Type => typeof(T);

        /// <summary>
        /// An event which is triggered whenever the value in this box changes. Args are The box, the old value, and the new value.
        /// </summary>
        public event Action<BaseBox<T>, T?, T?>? BoxChanged;
    }

    /// <summary>
    /// A class which boxes a value.
    /// </summary>
    /// <typeparam name="T">The type of the value to box.</typeparam>
    public class Box<T>
        : BaseBox<T>
    {
        /// <summary>
        /// The value this box contains.
        /// </summary>
        public override T? Value { get; set; }
    }

    /// <summary>
    /// A string with an associated generic type parameter
    /// </summary>
    /// <remarks>
    /// This may seem a little odd as the type parameter is not used. This type exists so that you can store a TypedName&lt;T&gt; somewhere and unambiguously refer to a box or property. The type parameter really exists for compiler inferencing
    /// </remarks>
    /// <typeparam name="T"></typeparam>
    [Serializable]
    public struct TypedName<T>
    {
        /// <summary>
        /// The name
        /// </summary>
        public string Name { get; }

        /// <summary>
        /// Construct a new TypedName
        /// </summary>
        /// <param name="name"></param>
        public TypedName(string name)
        {
            Name = name;
        }

        /// <summary>
        /// Append a string to the name of a TypedName
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b">the string to append, if this is null or empty nothing will happen</param>
        /// <returns></returns>
        public static TypedName<T> operator +(TypedName<T> a, string b)
        {
            if (string.IsNullOrEmpty(b))
                return a;

            return new TypedName<T>(string.Format("{0}_{1}", a.Name, b));
        }

        /// <summary>
        /// Explicitly cast this name into a string
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public static explicit operator string(TypedName<T> name)
        {
            return name.Name;
        }

        /// <summary>
        /// Explicitly cast a string into a typed name
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        public static explicit operator TypedName<T>(string name)
        {
            return new TypedName<T>(name);
        }

        /// <summary>
        /// Explicitly cast this generically typed name into an non generically typed one
        /// </summary>
        /// <param name="name"></param>
        /// <returns></returns>
        //public static explicit operator NameWithType(TypedName<T> name)
        //{
           // return new NameWithType(name.Name, typeof(T));
        //}
    }

    /// <summary>
    /// A collection of named data which is both readable and writeable
    /// </summary>
    public interface INamedDataCollection : INamedDataConsumer, INamedDataProvider { }

    /// <summary>
    /// A collection of named data which is writeable
    /// </summary>
    public interface INamedDataConsumer
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="key"></param>
        /// <param name="value"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        void Set<T>(TypedName<T> key, T value);
    }

    /// <summary>
    /// A collection of named data which is readable
    /// </summary>
    public interface INamedDataProvider : IEnumerable<KeyValuePair<string, IBox>>
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="name"></param>
        /// <param name="useDefaultValue"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        T? GetValue<T>(TypedName<T> name, bool useDefaultValue = true);

        /// <summary>
        /// 
        /// </summary>
        /// <param name="name"></param>
        /// <param name="value"></param>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        bool TryGetValue<T>(TypedName<T> name, out T? value);
    }



    /// <summary>
    /// A binary heap which allows efficient querying and removal of the minimum item in the heap
    /// </summary>
    /// <typeparam name="T"></typeparam>
    public interface IMinHeap<T> : IEnumerable<T>
    {
        /// <summary>
        /// Number of items in the heap
        /// </summary>
        int Count { get; }

        /// <summary>
        /// Peek the minimum value on the heap
        /// </summary>
        T Minimum { get; }

        /// <summary>
        /// Add a new item to the heap
        /// </summary>
        /// <param name="item"></param>
        void Add(T item);

        /// <summary>
        /// Add a lot of items to the heap (more efficient than calling Add(item) lots)
        /// </summary>
        /// <param name="items"></param>
        void Add(IEnumerable<T> items);

        /// <summary>
        /// Remove the minimum item from the heap
        /// </summary>
        /// <returns></returns>
        T RemoveMin();

        /// <summary>
        /// Remove the item at the given index
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        T RemoveAt(int index);

        /// <summary>
        /// Get the index of a given item (or -1 if cannot be found)
        /// </summary>
        /// <param name="item"></param>
        /// <returns></returns>
        int IndexOf(T item);

        /// <summary>
        /// Get the index of the first item which matches the given predicate (or -1 if cannot be found)
        /// </summary>
        /// <param name="predicate"></param>
        /// <returns></returns>
        int IndexOf(Predicate<T> predicate);

        /// <summary>
        /// Remove all items from the heap
        /// </summary>
        void Clear();
    }

    public class MinHeap<T>
        : IMinHeap<T>
    {
        #region fields and properties
        private readonly List<T> _heap;
        private readonly IComparer<T> _comparer;

        /// <summary>
        /// Get the number of items in this heap
        /// </summary>
        public int Count => _heap.Count;

        /// <summary>
        /// Get the minimum value in this heap
        /// </summary>
        public T Minimum => _heap[0];

        public bool AllowHeapResize { get; set; } = true;
        #endregion

        #region constructors
        /// <summary>
        /// 
        /// </summary>
        public MinHeap()
            : this(64)
        {

        }

        /// <summary>
        /// 
        /// </summary>
        public MinHeap(int capacity)
            : this(capacity, Comparer<T>.Default)
        {
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="capacity"></param>
        /// <param name="comparer"></param>
        public MinHeap(int capacity, IComparer<T> comparer)
        {
            if (capacity <= 0)
                throw new ArgumentOutOfRangeException(nameof(capacity));

            _heap = new List<T>(capacity);
            _comparer = comparer;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="capacity"></param>
        /// <param name="comparison"></param>
        public MinHeap(int capacity, Comparison<T> comparison)
            : this(capacity, Comparer<T>.Create(comparison))
        {
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="comparer"></param>
        public MinHeap(IComparer<T> comparer)
        {
            _heap = new List<T>();
            _comparer = comparer;
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="comparison"></param>
        public MinHeap(Comparison<T> comparison)
            : this(Comparer<T>.Create(comparison))
        {
        }
        #endregion

        #region add
        /// <summary>
        /// 
        /// </summary>
        /// <param name="item"></param>
        /// <exception cref="NotImplementedException"></exception>
        public void Add(T item)
        {
            if (!AllowHeapResize && _heap.Count == _heap.Capacity)
                throw new InvalidOperationException("Heap is full and resizing is disabled");

            _heap.Add(item);
            BubbleUp(_heap.Count - 1);

            DebugCheckHeapProperty();
        }

        /// <summary>
        /// Add a large number of items to the heap. This is more efficient that simply calling add on each item individually
        /// </summary>
        /// <param name="items"></param>
        public void Add(IEnumerable<T> items)
        {
            if (items == null)
                throw new ArgumentNullException(nameof(items));

            _heap.AddRange(items);
            Heapify();

            DebugCheckHeapProperty();
        }

        /// <summary>
        /// Establish the heap property (use this if you mutate an item already in the heap)
        /// </summary>
        public void Heapify()
        {
            // Using sorting is tempting, for it's sheer simplicity:
            //    _heap.Sort(_comparer);
            // There's also the possibility the framework implemented sorting algorithm is way better/faster than my heapify function in practical cases.
            // Benchmarking reveals sorting to *always* be slower, on both tiny heaps (10 items) and massive heaps (1000000 items)

            for (var i = _heap.Count - 1; i >= 0; i--)
                TrickleDown(i);

            DebugCheckHeapProperty();
        }

        /// <summary>
        /// Establish the heap property (use this if you mutate an item already in the heap)
        /// </summary>
        /// <param name="mutated">The index of the item which was mutated</param>
        public void Heapify(int mutated)
        {
            if (mutated < 0 || mutated >= Count)
                throw new IndexOutOfRangeException(nameof(mutated));

            //We either need to move this item up or down the heap. Try trickling down, and if that does nothing bubble up instead
            if (TrickleDown(mutated) == mutated)
                BubbleUp(mutated);
        }
        #endregion

        #region remove
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        /// <exception cref="NotImplementedException"></exception>
        public T RemoveMin()
        {
            return RemoveAt(0);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="index"></param>
        /// <returns></returns>
        /// <exception cref="InvalidOperationException"></exception>
        /// <exception cref="ArgumentOutOfRangeException"></exception>
        public T RemoveAt(int index)
        {
            if (index < 0 || index > _heap.Count)
                throw new ArgumentOutOfRangeException(nameof(index));

            var removed = _heap[index];

            //Move the last item into the first position
            _heap[index] = _heap[^1];
            _heap.RemoveAt(_heap.Count - 1);

            if (_heap.Count > 0 && index < _heap.Count)
                Heapify(0);

            DebugCheckHeapProperty();

            return removed;
        }

        /// <summary>
        /// 
        /// </summary>
        public void Clear()
        {
            _heap.Clear();
        }
        #endregion

        #region private helpers
        private void BubbleUp(int index)
        {
            while (index > 0)
            {
                var parent = ParentIndex(index);
                if (IsLessThan(_heap[index], _heap[parent]))
                {
                    Swap(parent, index);
                    index = parent;
                }
                else
                    break;
            }
        }

        private int TrickleDown(int index)
        {
            // This code was automatically converted to iteration instead of tail recursion
            // WTB C# tail call keyword!
            /* if (index >= _heap.Count)
                throw new ArgumentException();
            int smallestChildIndex = SmallestChildSmallerThan(index, _heap[index]);
            if (smallestChildIndex == -1)
                return index;
            Swap(smallestChildIndex, index);
            return TrickleDown(smallestChildIndex); */

            while (true)
            {
                if (index >= _heap.Count)
                    throw new ArgumentException();

                var smallestChildIndex = SmallestChildSmallerThan(index, _heap[index]);
                if (smallestChildIndex == -1)
                    return index;

                Swap(smallestChildIndex, index);
                index = smallestChildIndex;
            }
        }

        // ReSharper disable once MemberCanBeMadeStatic.Local
        private void DebugCheckHeapProperty()
        {
            //// ReSharper disable once InvokeAsExtensionMethod
            //if (Enumerable.Any(_heap, t => IsLessThan(t, Minimum)))
            //    throw new Exception("Heap property violated");
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private bool IsLessThan(T a, T b)
        {
            return _comparer.Compare(a, b) < 0;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static int ParentIndex(int i)
        {
            return (i - 1) / 2;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private void Swap(int a, int b)
        {
            var temp = _heap[a];
            _heap[a] = _heap[b];
            _heap[b] = temp;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static int LeftChild(int i)
        {
            return 2 * i + 1;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static int RightChild(int i)
        {
            return 2 * i + 2;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private int SmallestChildSmallerThan(int i, T item)
        {
            var leftChildIndex = LeftChild(i);
            var rightChildIndex = RightChild(i);

            var smallest = -1;
            if (leftChildIndex < _heap.Count)
                smallest = leftChildIndex;
            if (rightChildIndex < _heap.Count && IsLessThan(_heap[rightChildIndex], _heap[leftChildIndex]))
                smallest = rightChildIndex;

            if (smallest > -1 && IsLessThan(_heap[smallest], item))
                return smallest;

            return -1;
        }
        #endregion

        #region searching
        /// <summary>
        /// 
        /// </summary>
        /// <param name="item"></param>
        /// <returns></returns>
        public int IndexOf(T item)
        {
            return _heap.IndexOf(item);
        }

        /// <summary>
        /// 
        /// </summary>
        /// <param name="predicate"></param>
        /// <returns></returns>
        public int IndexOf(Predicate<T> predicate)
        {
            return _heap.FindIndex(predicate);
        }
        #endregion

        IEnumerator IEnumerable.GetEnumerator()
        {
            return ((IEnumerable<T>)this).GetEnumerator();
        }

        /// <summary>
        /// Enumerate this heap *in no particular order*
        /// </summary>
        /// <returns></returns>
        IEnumerator<T> IEnumerable<T>.GetEnumerator()
        {
            return _heap.GetEnumerator();
        }
    }



    public interface IValueGenerator
    {
        float MaxValue { get; }
        float MinValue { get; }

        float SelectFloatValue(Func<double> random, INamedDataCollection data);
        int SelectIntValue(Func<double> random, INamedDataCollection data);
    }

    public enum LineParallelism
    {
        /// <summary>
        /// Lines are not parallel
        /// </summary>
        None,

        /// <summary>
        /// Lines are parallel (i.e. point in the same direction)
        /// </summary>
        Parallel,

        /// <summary>
        /// Line are collinear (i.e. point in the same direction and pass through the same point)
        /// </summary>
        Collinear
    }

    public struct Ray2
    {
        /// <summary>
        /// Specifies the starting point of the Ray.
        /// </summary>
        public Float2 Position;

        /// <summary>
        /// Unit vector specifying the direction the Ray is pointing.
        /// </summary>
        public Float2 Direction;

        /// <summary>
        /// Creates a new instance of Ray.
        /// </summary>
        /// <param name="position">The starting point of the Ray.</param><param name="direction">Unit vector describing the direction of the Ray.</param>
        public Ray2(Float2 position, Float2 direction)
        {
            Position = position;
            Direction = direction;
        }

        #region equality
        /// <summary>
        /// Determines whether two instances of Ray are equal.
        /// </summary>
        /// <param name="a">The object to the left of the equality operator.</param><param name="b">The object to the right of the equality operator.</param>
        public static bool operator ==(Ray2 a, Ray2 b)
        {
            return a.Equals(b);
        }

        /// <summary>
        /// Determines whether two instances of Ray are not equal.
        /// </summary>
        /// <param name="a">The object to the left of the inequality operator.</param><param name="b">The object to the right of the inequality operator.</param>
        public static bool operator !=(Ray2 a, Ray2 b)
        {
            return !a.Equals(b);
        }

        /// <summary>
        /// Determines whether the specified Ray is equal to the current Ray.
        /// </summary>
        /// <param name="other">The Ray to compare with the current Ray.</param>
        public bool Equals(Ray2 other)
        {
            return Position.Equals(other.Position) && Direction.Equals(other.Direction);
        }

        /// <summary>
        /// Determines whether two instances of Ray are equal.
        /// </summary>
        /// <param name="obj">The Object to compare with the current Ray.</param>
        public override bool Equals(object obj)
        {
            return obj is Ray2 ray2 && Equals(ray2);
        }

        /// <summary>
        /// Gets the hash code for this instance.
        /// </summary>
        public override int GetHashCode()
        {
            // ReSharper disable NonReadonlyFieldInGetHashCode

            var hash = 17;
            hash = hash * 31 + Position.GetHashCode();
            hash = hash * 31 + Direction.GetHashCode();
            return hash;

            // ReSharper restore NonReadonlyFieldInGetHashCode
        }
        #endregion

        /// <summary>
        /// Returns a String that represents the current Ray.
        /// </summary>
        public override string ToString()
        {
            return string.Format("Position:{0},Direction:{1}", Position, Direction);
        }

        #region closest point
        /// <summary>
        /// Calculate the closest point on this ray to the given point
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        public Float2 ClosestPoint(Float2 point)
        {
            return ClosestPoint(ref point, out var _);
        }

        /// <summary>
        /// Calculate the closest point on this ray to the given point
        /// </summary>
        /// <param name="point"></param>
        /// <param name="result">distance along the ray at which the closest point lies</param>
        /// <returns></returns>
        public Float2 ClosestPoint(ref Float2 point, out float result)
        {
            ClosestPointDistanceAlongLine(ref point, out result);
            return Position + Direction * result;
        }

        /// <summary>
        /// Gets how far along this line the closest point is (in units of direction length)
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        public float ClosestPointDistanceAlongLine(Float2 point)
        {
            ClosestPointDistanceAlongLine(ref point, out var dist);
            return dist;
        }

        /// <summary>
        /// Gets how far along this line the closest point is (in units of direction length)
        /// </summary>
        /// <param name="point"></param>
        /// <param name="distance"></param>
        /// <returns></returns>
        public void ClosestPointDistanceAlongLine(ref Float2 point, out float distance)
        {
            var direction = Direction;
            var lengthSq = direction.SquareMagnitude();

            distance = Float2.Dot((point - Position), direction) / lengthSq;
        }

        public float DistanceToPoint(Float2 point)
        {
            DistanceToPoint(ref point, out var result);
            return result;
        }

        public void DistanceToPoint(ref Float2 point, out float distance)
        {
            distance = Float2.Cross(Direction, point - Position) / Direction.Magnitude();
        }
        #endregion

        #region intersection
        public LinesIntersection2? Intersects(Ray2 ray, out LineParallelism parallelism)
        {
            //http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect

            var p = Position;
            var r = Direction;

            var q = ray.Position;
            var s = ray.Direction;

            // It isn't maths if the variable names make any sense
            // ReSharper disable InconsistentNaming
            var RxS = Float2.Cross(r,s);
            var QmP = q - p;
            // ReSharper restore InconsistentNaming

            if (Math.Abs(RxS - 0) < 0.001)
            {
                // ReSharper disable InconsistentNaming
                var QmPxR = Float2.Cross(QmP,r);
                // ReSharper restore InconsistentNaming
                parallelism = Math.Abs(QmPxR - 0) < 0.001 ? LineParallelism.Collinear : LineParallelism.Parallel;
                return null;
            }

            var t = Float2.Cross(QmP, s) / RxS;
            var u = Float2.Cross(QmP, r) / RxS;

            var point = p + (r * t);

            parallelism = LineParallelism.None;
            return new LinesIntersection2(point, t, u);
        }

        public LinesIntersection2? Intersects(Ray2 ray)
        {
            return Intersects(ray, out var _);
        }

        /// <summary>
        /// Checks whether the Ray intersects a specified Bounds2.
        /// </summary>
        /// <param name="box">The Bounds2 to check for intersection with the Ray.</param>
        public float? Intersects(Bounds2 box)
        {
            Intersects(ref box, out var result);
            return result;
        }

        /// <summary>
        /// Checks whether the current Ray intersects a Bounds2.
        /// </summary>
        /// <param name="box">The Bounds2 to check for intersection with.</param><param name="result">[OutAttribute] Distance at which the ray intersects the BoundingBox or null if there is no intersection.</param>
        public void Intersects(ref Bounds2 box, out float? result)
        {
            const float epsilon = 1e-6f;

            float? tMin = null, tMax = null;

            if (Math.Abs(Direction.x) < epsilon)
            {
                if (Position.x < box.Min.x || Position.x > box.Max.x)
                {
                    result = null;
                    return;
                }
            }
            else
            {
                tMin = (box.Min.x - Position.x) / Direction.x;
                tMax = (box.Max.x - Position.x) / Direction.x;

                if (tMin > tMax)
                {
                    var temp = tMin;
                    tMin = tMax;
                    tMax = temp;
                }
            }

            if (Math.Abs(Direction.y) < epsilon)
            {
                if (Position.y < box.Min.y || Position.y > box.Max.y)
                {
                    result = null;
                    return;
                }
            }
            else
            {
                var tMinY = (box.Min.y - Position.y) / Direction.y;
                var tMaxY = (box.Max.y - Position.y) / Direction.y;

                if (tMinY > tMaxY)
                {
                    var temp = tMinY;
                    tMinY = tMaxY;
                    tMaxY = temp;
                }

                if ((tMin.HasValue && tMin > tMaxY) || (tMax.HasValue && tMinY > tMax))
                {
                    result = null;
                    return;
                }

                if (!tMin.HasValue || tMinY > tMin) tMin = tMinY;
                if (!tMax.HasValue || tMaxY < tMax) tMax = tMaxY;
            }

            // having a positive tMin and a negative tMax means the ray is inside the box
            // we expect the intesection distance to be 0 in that case
            if ((tMin.HasValue && tMin < 0) && tMax > 0)
            {
                result = 0;
                return;
            }

            // a negative tMin means that the intersection point is behind the ray's origin
            // we discard these as not hitting the AABB
            if (tMin < 0)
            {
                result = null;
                return;
            }

            result = tMin;
        }

        ///// <summary>
        ///// Checks whether the Ray intersects a specified BoundingSphere.
        ///// </summary>
        ///// <param name="sphere">The BoundingSphere to check for intersection with the Ray.</param>
        //public float? Intersects(BoundingSphere sphere)
        //{
        //    float? result;
        //    Intersects(ref sphere, out result);
        //    return result;
        //}

        ///// <summary>
        ///// Checks whether the current Ray intersects a BoundingSphere.
        ///// </summary>
        ///// <param name="sphere">The BoundingSphere to check for intersection with.</param><param name="result">[OutAttribute] Distance at which the ray intersects the BoundingSphere or null if there is no intersection.</param>
        //public void Intersects(ref BoundingSphere sphere, out float? result)
        //{
        //    // Find the vector between where the ray starts the the sphere's centre
        //    Float2 difference = sphere.Center - Position;

        //    float differenceLengthSquared = difference.LengthSquared();
        //    float sphereRadiusSquared = sphere.Radius * sphere.Radius;

        //    // If the distance between the ray start and the sphere's centre is less than
        //    // the radius of the sphere, it means we've intersected. N.B. checking the LengthSquared is faster.
        //    if (differenceLengthSquared < sphereRadiusSquared)
        //    {
        //        result = 0.0f;
        //        return;
        //    }

        //    var distanceAlongRay = Float2.Dot(Direction, difference);
        //    // If the ray is pointing away from the sphere then we don't ever intersect
        //    if (distanceAlongRay < 0)
        //    {
        //        result = null;
        //        return;
        //    }

        //    // Next we kinda use Pythagoras to check if we are within the bounds of the sphere
        //    // if x = radius of sphere
        //    // if y = distance between ray position and sphere centre
        //    // if z = the distance we've travelled along the ray
        //    // if x^2 + z^2 - y^2 < 0, we do not intersect
        //    float dist = sphereRadiusSquared + distanceAlongRay * distanceAlongRay - differenceLengthSquared;

        //    result = (dist < 0) ? null : distanceAlongRay - (float?)Math.Sqrt(dist);
        //}
        #endregion

        /// <summary>
        /// Checks if the given point is to the left of the line (from an observer standing at the start looking along the line)
        /// </summary>
        /// <param name="point"></param>
        /// <param name="epsilon"></param>
        /// <returns></returns>
        public bool IsLeft(Float2 point, float epsilon)
        {
            return DistanceToPoint(point) > -epsilon;
        }

        public LineParallelism Parallelism(Ray2 ray)
        {
            //http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect

            var p = Position;
            var r = Direction;

            var q = ray.Position;
            var s = ray.Direction;

            // It isn't maths if the variable names make any sense
            // ReSharper disable InconsistentNaming
            var RxS = Float2.Cross(r,s);
            var QmP = q - p;
            // ReSharper restore InconsistentNaming

            if (Math.Abs(RxS - 0) < 0.001)
            {
                // ReSharper disable InconsistentNaming
                var QmPxR = Float2.Cross(QmP,r);
                // ReSharper restore InconsistentNaming
                return Math.Abs(QmPxR - 0) < 0.001 ? LineParallelism.Collinear : LineParallelism.Parallel;
            }

            return LineParallelism.None;
        }

        /// <summary>
        /// Given a distance along the line (in units of line length) get the point this distance evaluates to
        /// </summary>
        /// <param name="t"></param>
        /// <returns></returns>
        public Float2 PointAlongLine(float t)
        {
            return Position + Direction * t;
        }
    }

    public struct LineSegment2
    {
        /// <summary>
        /// Specifies the starting point of the segment.
        /// </summary>
        public Float2 Start;

        /// <summary>
        /// Specifies the ending point of the segment.
        /// </summary>
        public Float2 End;

        /// <summary>
        /// Return a new Ray2, pointing along this segment (with normalized direction vector)
        /// </summary>
        /// <returns></returns>
        public Ray2 Line
        {
            get { return new Ray2(Start, (End - Start).Normalize()); }
        }

        /// <summary>
        /// Return a new Ray2, pointing along this segment (with non normalized direction vector)
        /// </summary>
        /// <returns></returns>
        public Ray2 LongLine => new Ray2(Start, End - Start);

        /// <summary>
        /// Creates a new instance of LineSegment2.
        /// </summary>
        /// <param name="start">The starting point of the Ray.</param><param name="end">end of the segment</param>
        public LineSegment2(Float2 start, Float2 end)
        {
            Start = start;
            End = end;
        }

        #region equality
        /// <summary>
        /// Determines whether two instances of LineSegment2 are equal.
        /// </summary>
        /// <param name="a">The object to the left of the equality operator.</param><param name="b">The object to the right of the equality operator.</param>
        public static bool operator ==(LineSegment2 a, LineSegment2 b)
        {
            return a.Equals(b);
        }

        /// <summary>
        /// Determines whether two instances of LineSegment2 are not equal.
        /// </summary>
        /// <param name="a">The object to the left of the inequality operator.</param><param name="b">The object to the right of the inequality operator.</param>
        public static bool operator !=(LineSegment2 a, LineSegment2 b)
        {
            return !a.Equals(b);
        }

        /// <summary>
        /// Determines whether the specified LineSegment2 is equal to the current Ray.
        /// </summary>
        /// <param name="other">The LineSegment2 to compare with the current LineSegment2.</param>
        public bool Equals(LineSegment2 other)
        {
            return Start.Equals(other.Start) && End.Equals(other.End);
        }

        /// <summary>
        /// Determines whether two instances of Ray are equal.
        /// </summary>
        /// <param name="obj">The Object to compare with the current Ray.</param>
        public override bool Equals(object obj)
        {
            return obj is LineSegment2 segment2 && Equals(segment2);
        }

        /// <summary>
        /// Gets the hash code for this instance.
        /// </summary>
        public override int GetHashCode()
        {
            // ReSharper disable NonReadonlyFieldInGetHashCode

            var hash = 17;
            hash = hash * 31 + Start.GetHashCode();
            hash = hash * 31 + End.GetHashCode();
            return hash;

            // ReSharper restore NonReadonlyFieldInGetHashCode
        }
        #endregion

        /// <summary>
        /// Returns a String that represents the current Ray.
        /// </summary>
        public override string ToString()
        {
            return $"Start:{Start},End:{End}";
        }

        #region closest point
        /// <summary>
        /// Calculate the closest point on this ray to the given point
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        public Float2 ClosestPoint(Float2 point)
        {
            return ClosestPoint(ref point, out var t);
        }

        /// <summary>
        /// Calculate the closest point on this ray to the given point
        /// </summary>
        /// <param name="point"></param>
        /// <param name="result">distance along the ray at which the closest point lies</param>
        /// <returns></returns>
        public Float2 ClosestPoint(ref Float2 point, out float result)
        {
            var pos = LongLine.ClosestPoint(ref point, out result);

            if (result > 1)
            {
                result = 1;
                return End;
            }

            if (result < 0)
            {
                result = 0;
                return Start;
            }

            return pos;
        }

        /// <summary>
        /// Gets how far along this line the closest point is (in units of direction length)
        /// </summary>
        /// <param name="point"></param>
        /// <returns></returns>
        public float ClosestPointDistanceAlongSegment(Float2 point)
        {
            ClosestPointDistanceAlongSegment(ref point, out var dist);
            return dist;
        }

        /// <summary>
        /// Gets how far along this line the closest point is (in units of direction length)
        /// </summary>
        /// <param name="point"></param>
        /// <param name="distance"></param>
        /// <returns></returns>
        public void ClosestPointDistanceAlongSegment(ref Float2 point, out float distance)
        {
            LongLine.ClosestPointDistanceAlongLine(ref point, out distance);

            distance = Math.Clamp(distance, 0, 1);
        }

        public float DistanceToPoint(Float2 point)
        {
            DistanceToPoint(ref point, out var result);
            return result;
        }

        public void DistanceToPoint(ref Float2 point, out float distance)
        {
            distance = (point - ClosestPoint(point)).Magnitude();
        }
        #endregion

        #region intersection
        public LinesIntersection2? Intersects(Ray2 ray, out LineParallelism parallelism)
        {
            var intersection = LongLine.Intersects(ray, out parallelism);

            if (!intersection.HasValue)
                return null;

            if (intersection.Value.DistanceAlongA <= 0 || intersection.Value.DistanceAlongA >= 1)
                return null;

            return intersection;
        }

        public LinesIntersection2? Intersects(Ray2 ray)
        {
            return Intersects(ray, out var _);
        }

        public LinesIntersection2? Intersects(LineSegment2 segment, out LineParallelism parallelism)
        {
            var intersection = LongLine.Intersects(segment.LongLine, out parallelism);

            if (!intersection.HasValue)
                return null;

            if (intersection.Value.DistanceAlongA < 0 || intersection.Value.DistanceAlongA > 1)
                return null;

            if (intersection.Value.DistanceAlongB < 0 || intersection.Value.DistanceAlongB > 1)
                return null;

            return intersection;
        }

        public LinesIntersection2? Intersects(LineSegment2 segment)
        {
            return Intersects(segment, out LineParallelism _);
        }
        #endregion

        public LineSegment2 Transform(Matrix4x4 transform)
        {
            Float3 sf = new Float3(Start.x, 0, Start.y);
            sf = transform.MultiplyPoint3x4(sf);
            Float2 s = new Float2(sf.x, sf.z);

            //var s = Vector3.Transform(Start.x_Y(0), transform).xZ();
            Float3 ef = new Float3(End.x, 0, End.y);
            ef = transform.MultiplyPoint3x4(ef);

            Float2 e = new Float2(ef.x, ef.z);

            return new LineSegment2(s, e);
        }
    }

    public struct LinesIntersection2
    {
        /// <summary>
        /// The position where the two rays intersect
        /// </summary>
        public readonly Float2 Position;

        /// <summary>
        /// The distance along ray A. Units are in ray lengths, so 0 indicates the start, 1 indicates the end.
        /// </summary>
        public readonly float DistanceAlongA;

        /// <summary>
        /// The distance along ray B. Units are in ray lengths, so 0 indicates the start, 1 indicates the end.
        /// </summary>
        public readonly float DistanceAlongB;

        public LinesIntersection2(Float2 position, float distanceAlongLineA, float distanceAlongLineB)
        {
            Position = position;
            DistanceAlongA = distanceAlongLineA;
            DistanceAlongB = distanceAlongLineB;
        }
    }



    public class NetworkBuilder
    {
        private Quadtree<TraceVertex> _vertices;
        private Quadtree<TracerEdge> _edges;

        private readonly HashSet<Streamline> _streams;

        private bool _begun;
        private Float2 _min;
        private Float2 _max;

        public NetworkBuilder()
        {
            _streams = new HashSet<Streamline>();
        }

        #region finalisation
        public Network Result
        {
            get
            {
                return new Network(
                    Vertices()
                );
            }
        }

        public void Reduce()
        {
            foreach (var streamline in _streams)
                LinearReduction(streamline);
        }
        #endregion

        #region building
        public void Begin(Float2 min, Float2 max)
        {
            if (_begun)
                throw new InvalidOperationException("Cannot call Begin twice");

            _min = min;
            _max = max;

            Float2 center = min + max;
            center *= 0.5f;
            Float2 size = max - min;

            //TODO: Figure out what this is or should default to, if not zero.
            int maxDepth = 0;

            _vertices = new Quadtree<TraceVertex>(new Bounds2(center, size), 63, maxDepth);
            _edges = new Quadtree<TracerEdge>(new Bounds2(center, size), 31, maxDepth);

            _begun = true;
        }

        public void Build(TracingConfiguration config, Func<double> random, INamedDataCollection metadata, Float2 min, Float2 max)
        {

            if (!_begun)
            {
                Begin(min, max);
                AddBoundary(min, max);
            }

            var extent = max - min;
            var eigens = config.TensorField.Presample(min, max, (uint)Math.Max(extent.x, extent.y));

            var count = (int)((extent.x * extent.y) / 2500);

            Func<Float2, bool> isOutOfBounds = v => v.x > max.x || v.x < min.x || v.y > max.y || v.y < min.y;

            var seeds = RandomTracingSeedsInBounds(random, count, eigens.MajorEigenVectors, eigens.MinorEigenVectors, min, max, isOutOfBounds);

            Build(seeds, min, max, config.SeparationField, true, true, config.SegmentLength, config.MergeDistance, config.CosineSearchConeAngle, isOutOfBounds, null, s =>
            {
                s.Width = (uint)config.RoadWidth.SelectIntValue(random, metadata);

                LinearReduction(s);
            });
        }

        public void Build(TracingConfiguration config, Func<double> random, INamedDataCollection metadata, TracedRegion region)
        {

            //var extent = region.Max - region.Min;
            //var eigens = config.TensorField.Presample(new Float2(region.Min.x, region.Min.y), new Float2(region.Max.x, region.Max.y), (int)Math.Max(extent.x, extent.y));

            var ex = _max - _min;
            var eigens = config.TensorField.Presample(_min, _max, (uint)Math.Max(ex.x, ex.y));

            var seeds = TracingSeedsAlongTracerEdge(region, config.SeparationField, eigens.MajorEigenVectors, eigens.MinorEigenVectors);

            Build(seeds, region.Min, region.Max,
                config.SeparationField,
                true, true,
                config.MergeDistance,
                config.SegmentLength,
                config.CosineSearchConeAngle,
                p => !region.PointInPolygon(p), e => e.Streamline.Region == region,
                s => {
                    s.Width = (uint)config.RoadWidth.SelectIntValue(random, metadata);
                    s.Region = region;

                    LinearReduction(s);
                });
        }

        private void Build(IEnumerable<TracingSeed> initialTracingSeeds, Float2 min, Float2 max, BaseScalarField separation, bool forward, bool backward, float maxSegmentLength, float mergeDistance, float cosineSearchAngle, Func<Float2, bool> isOutOfBounds, Func<TracerEdge, bool> edgeFilter, Action<Streamline> streamCreated)
        {
            var seeds = new MinHeap<KeyValuePair<float, TracingSeed>>(1024, new KeyComparer<float, TracingSeed>());
            foreach (var initialTracingSeed in initialTracingSeeds)
                AddTracingSeed(seeds, initialTracingSeed);

            //Trace out roads for every single seed
            while (seeds.Count > 0)
            {
                var s = RemoveTracingSeed(seeds, separation, cosineSearchAngle, edgeFilter);
                if (!s.HasValue)
                    continue;

                if (forward)
                {
                    var stream = CheckStream(Trace(s.Value, false, seeds, isOutOfBounds, maxSegmentLength, mergeDistance, cosineSearchAngle, separation));
                    if (stream != null)
                    {
                        _streams.Add(stream);
                        streamCreated(stream);
                    }
                }

                if (backward)
                {
                    var stream = CheckStream(Trace(s.Value, true, seeds, isOutOfBounds, maxSegmentLength, mergeDistance, cosineSearchAngle, separation));
                    if (stream != null)
                    {
                        _streams.Add(stream);
                        streamCreated(stream);
                    }
                }
            }
        }

        private void AddBoundary(Float2 min, Float2 max)
        {
            //Start boundary
            var boundary = new Streamline(CreateTraceVertex(min));

            //Loop around 3 sides
            InsertTracerEdge(boundary.Extend(CreateTraceVertex(new Float2(max.x, min.y))));
            InsertTracerEdge(boundary.Extend(CreateTraceVertex(new Float2(max.x, max.y))));
            InsertTracerEdge(boundary.Extend(CreateTraceVertex(new Float2(min.x, max.y))));

            //Close loop
            InsertTracerEdge(boundary.Extend(boundary.First));
        }
        #endregion

        #region streams
        private void LinearReduction(Streamline stream)
        {
            
            //Helper functions
            var visited = new HashSet<TracerEdge>();
            Func<TracerEdge, bool> predicate = e => e.Streamline == stream;
            Func<TraceVertex, TracerEdge> next = v => v.Edges.Where(predicate).Where(a => !visited.Contains(a)).SingleOrDefault(e => e.A.Equals(v));

            //Setup, we'll advance "end" forwards, reducing edges which are straight, until end is the end of the streamline
            var start = stream.First;
            var se = next(start);
            if (se == null)
                return;
            var end = se.B;
            var segmentDirection = (end.Position - start.Position).Normalize();

            while (end.EdgeCount != 1)
            {
                visited.Add(se);

                //If we hit a branching vertex skip forwards
                while (end.EdgeCount > 2)
                {
                    start = end;
                    se = next(start);
                    if (se == null)
                        return;
                    end = se.B;
                    segmentDirection = (end.Position - start.Position).Normalize();
                }


                var n = next(end);
                if (n == null)
                    return;

                var dir = (n.B.Position - start.Position).Normalize();

                if (Float2.Dot(segmentDirection, dir) > 0.999)
                {
                    //Reduce

                    //Delete old components
                    DeleteTracerEdge(se);
                    DeleteTracerEdge(n);
                    DeleteTraceVertex(end);

                    //Remove mid vertex from stream
                    stream.Remove(end);

                    //Create new edge
                    se = TracerEdge.Create(stream, start, n.B);
                    InsertTracerEdge(se);

                    //Move end pointer
                    end = n.B;
                }
                else
                {
                    start = end;
                    se = n;
                    end = se.B;
                    segmentDirection = (end.Position - start.Position).Normalize();
                }
            }
        }

        private Streamline CheckStream(Streamline stream)
        {
            if (stream == null)
                return null;

            if (stream.Count == 1)
            {
                DeleteStream(stream);
                return null;
            }

            return stream;
        }

        private void DeleteStream(Streamline streamline)
        {
            
            foreach (var vertex in streamline.Vertices)
            {
                var edge = vertex.Edges.SingleOrDefault(e => e.Streamline == streamline);
                if (edge != null)
                    DeleteTracerEdge(edge);
            }

            foreach (var vertex in streamline.Vertices)
            {
                if (vertex.EdgeCount == 0)
                    DeleteTraceVertex(vertex);
            }

            _streams.Remove(streamline);
        }

        private Streamline Trace(TracingSeed seed, bool reverse, MinHeap<KeyValuePair<float, TracingSeed>> seeds, Func<Float2, bool> isOutOfBounds, float maxSegmentLength, float mergeDistance, float cosineSearchAngle, BaseScalarField separation)
        {
            var maxSegmentLengthSquared = maxSegmentLength * maxSegmentLength;

            var seedingDistance = float.MaxValue;
            var direction = Float2.zero;
            var position = seed.Point;
            var stream = new Streamline(FindOrCreateTraceVertex(position, mergeDistance, cosineSearchAngle));

            //This is a weird way to do a for loop! What gives?
            //This is, in many respects, a better way to do it if you don't want i to be mutated within the loop
            //In this case I'm using it to pacify a persistent CodeContracts false positive (this loop is too complex for it to analyze, I guess?)
            foreach (var i in Enumerable.Range(0, 10000))
            {
                direction = seed.Field.TraceVectorField(position, direction, maxSegmentLength);
                if (i == 0)
                    direction *= reverse ? -1 : 1;

                //degenerate step check
                var segmentLength = direction.Magnitude();
                if (segmentLength < 0.00005f)
                    break;

                //Excessive step check
                if (segmentLength > maxSegmentLength)
                {
                    direction /= segmentLength * maxSegmentLength;
                    segmentLength = maxSegmentLength;
                }

                //Step along path
                position += direction;
                seedingDistance += segmentLength;

                //Bounds check
                if (isOutOfBounds(position))
                {
                    CreateTracerEdge(stream, position, direction.Normalize(), maxSegmentLength, maxSegmentLengthSquared, mergeDistance, cosineSearchAngle, skipDistanceCheck: true);
                    break;
                }

                //Create the segment and break if it says so
                if (CreateTracerEdge(stream, position, direction.Normalize(), maxSegmentLength, maxSegmentLengthSquared, mergeDistance, cosineSearchAngle))
                    break;

                //Accumulate seeds to trace into the alternative field
                var seedSeparation = separation.Sample(position);
                if (seedingDistance > seedSeparation)
                {
                    seedingDistance = 0;
                    AddTracingSeed(seeds, new TracingSeed(position, seed.AlternativeField, seed.Field));
                }
            }

            return stream;
        }
        #endregion

        public IEnumerable<TracedRegion> Regions()
        {
            return new TracedRegionBuilder(Vertices()).Regions();
        }

        #region edges
        private bool CreateTracerEdge(Streamline streamline, Float2 endPosition, Float2 direction, float segmentLength, float segmentLengthSquared, float mergeDistance, float cosineSearchAngle, bool skipDistanceCheck = false)
        {

            //Do not create an edge if this distance is too short
            float distanceSquared = Float2.Distance(streamline.Last.PositionField, endPosition);
            distanceSquared *= distanceSquared;
            if (!skipDistanceCheck &&  distanceSquared < segmentLengthSquared)
                return false;

            //Check if the position of the line has wandered close to an already existing vertex
            TraceVertex endTraceVertex = FindTraceVertex(endPosition, direction, mergeDistance, cosineSearchAngle, streamline.Last);

            //Check if this proposed new edge intersects another already existing edge
            Float2 intersectPosition;
            var intersect = FindIntersectingTracerEdge(streamline.Last.Position, endTraceVertex == null ? endPosition : endTraceVertex.Position, out intersectPosition);
            if (intersect != null)
            {
                //If we're close enough to either end, use that end instead of splitting the edge
                var dist = segmentLength * 0.2f;
                if ((intersectPosition - intersect.A.Position).Magnitude() < dist)
                    endTraceVertex = intersect.A;
                else if ((intersectPosition - intersect.B.Position).Magnitude() < dist)
                    endTraceVertex = intersect.B;
                else
                {
                    //Nope, we've gotta split the edge
                    TracerEdge aMid, midB;
                    endTraceVertex = intersect.Split(FindOrCreateTraceVertex(intersectPosition, dist, 0), out aMid, out midB);
                    DeleteTracerEdge(intersect);
                    InsertTracerEdge(aMid);
                    InsertTracerEdge(midB);
                }

                //Console.WriteLine(endPosition.GetHashCode() + " Found intersecting edge");
            }

            //If we've not found a vertex by any of the above, the line can continue
            bool endOfLine = endTraceVertex != null;

            //Haven't moved very far, trying to connect a vertex to itself
            if (endTraceVertex != null && endTraceVertex.Equals(streamline.Last))
                return false;

            //Didn't find one, create a new vertex
            if (endTraceVertex == null)
                endTraceVertex = CreateTraceVertex(endPosition);

            //Extend the streamline (endOfLine if for some reason an edge isn't created)
            if (!streamline.Contains(endTraceVertex) || streamline.First.Equals(endTraceVertex))
                endOfLine |= !InsertTracerEdge(streamline.Extend(endTraceVertex));

            //Wrapped around
            endOfLine |= streamline.First.Equals(streamline.Last);

            //Return an indication of if we have hit the end of the line by merging into another
            return endOfLine;
        }

        private bool InsertTracerEdge(TracerEdge e)
        {
            
            if (e == null)
                return false;

            _edges.Insert(new Bounds2(
                new Float2(Math.Min(e.A.Position.x, e.B.Position.x), Math.Min(e.A.Position.y, e.B.Position.y)),
                new Float2(Math.Max(e.A.Position.x, e.B.Position.x), Math.Max(e.A.Position.y, e.B.Position.y))
            ), e);

            return true;
        }

        /// <summary>
        /// Find edges close to given position
        /// </summary>
        /// <param name="position"></param>
        /// <param name="radius"></param>
        /// <returns></returns>
        private IEnumerable<TracerEdge> FindTracerEdges(Float2 position, float radius)
        {
            
            return _vertices.Intersects(
                new Bounds2(
                    new Float2(position.x - radius, position.y - radius),
                    new Float2(position.x + radius, position.y + radius)
                )
            ).SelectMany(v => v.Edges).Distinct();
        }

        private TracerEdge FindIntersectingTracerEdge(Float2 a, Float2 b, out Float2 intersectPosition)
        {
            var min = new Float2(Math.Min(a.x, b.x), Math.Min(a.y, b.y));
            var max = new Float2(Math.Max(a.x, b.x), Math.Max(a.y, b.y));

            //Select the all intersections (by checking all edges in octree result)
            var segment = new LineSegment2(a, b);
            var best = (from candidate in _edges.Intersects(new Bounds2(min, max))
                        let intersectionMaybe = new LineSegment2(candidate.A.Position, candidate.B.Position).Intersects(segment)
                        where intersectionMaybe.HasValue
                        let intersection = intersectionMaybe.Value
                        select new KeyValuePair<LinesIntersection2, TracerEdge>(intersection, candidate));

            intersectPosition = Float2.zero;
            TracerEdge bestTracerEdge = null;
            var bestScore = float.MaxValue;
            foreach (var keyValuePair in best)
            {
                if (keyValuePair.Key.DistanceAlongB < bestScore)
                {
                    intersectPosition = keyValuePair.Key.Position;
                    bestTracerEdge = keyValuePair.Value;
                    bestScore = keyValuePair.Key.DistanceAlongB;
                }
            }

            return bestTracerEdge;
        }

        private void DeleteTracerEdge(TracerEdge e)
        {
            
            //Remove from vertices
            e.A.Remove(e);
            e.B.Remove(e);

            //Create bounding rectangle (with 1 padding in either direction)
            var a = e.A.Position;
            var b = e.B.Position;
            var min = new Float2(Math.Min(a.x, b.x) - 1, Math.Min(a.y, b.y) - 1);
            var max = new Float2(Math.Max(a.x, b.x) + 1, Math.Max(a.y, b.y) + 1);

            //Remove from quadtree index
            if (!_edges.Remove(new Bounds2(min, max), e)) { }
                //throw new InstanceNotFoundException("Failed to delete edge");
        }
        #endregion

        #region vertices
        private TraceVertex FindOrCreateTraceVertex(Float2 position, float mergeDistance, float cosineSearchAngle)
        {
            
            return FindTraceVertex(position, Float2.zero, mergeDistance, cosineSearchAngle, null)
                ?? CreateTraceVertex(position);
        }

        private TraceVertex FindTraceVertex(Float2 position, Float2 direction, float radius, float cosineSearchAngle, TraceVertex skip)
        {
           
            var candidates = _vertices.Intersects(new Bounds2(new Float2(position.x - radius, position.y - radius), new Float2(position.x + radius, position.y + radius)));

            TraceVertex closest = null;
            var closestDistanceSqr = float.MaxValue;
            foreach (var candidate in candidates)
            {
                if (candidate.Equals(skip))
                    continue;

                var v = candidate.Position - position;

                //Select the closest candidate
                var dSqr = v.SquareMagnitude();
                if (dSqr > closestDistanceSqr)
                    continue;

                //Only select vertices which are within a cone in the direction of travel
                if (direction != Float2.zero)
                {
                    var dot = Float2.Dot(v / (float)Math.Sqrt(dSqr), direction);
                    if (dot < cosineSearchAngle)
                        continue;
                }

                closestDistanceSqr = dSqr;
                closest = candidate;
            }

            //The query is a circle, but we checked a rectangle, reject if it's in the rect but outside the circle
            if (Math.Sqrt(closestDistanceSqr) > radius)
                return null;

            return closest;
        }

        private TraceVertex CreateTraceVertex(Float2 position)
        {
            var overlaps = _vertices.ContainedBy(new Bounds2(
                new Float2(position.x - 1, position.y - 1),
                new Float2(position.x + 1, position.y + 1)
            ));
            if (overlaps.Any(a => a.Position == position))
                throw new InvalidOperationException();

            var vertex = new TraceVertex(position);

            _vertices.Insert(
                new Bounds2(
                    new Float2(position.x, position.y),
                    new Float2(position.x, position.y)
                ),
                vertex
            );

            return vertex;
        }

        private bool DeleteTraceVertex(TraceVertex vertex)
        {
            var pos = new Float2(vertex.Position.x, vertex.Position.y);
            return _vertices.Remove(new Bounds2(pos, pos), vertex);
        }
        #endregion

        #region seeds
        private static IEnumerable<TracingSeed> TracingSeedsAlongTracerEdge(TracedRegion region, BaseScalarField distanceField, IFloat2Field major, IFloat2Field minor)
        {
            float d = 0;
            Float2? previous = null;
            foreach (var vertex in region.Vertices)
            {
                if (previous.HasValue)
                {
                    var pos = previous.Value;
                    var v = vertex - previous.Value;
                    var length = v.Magnitude();
                    var dir = v / length;

                    for (int i = 0; i < length; i++)
                    {
                        var separation = distanceField.Sample(pos + dir * i);
                        d += separation;
                        if (d >= separation)
                        {
                            yield return new TracingSeed(pos + dir * i, major, minor);
                            d -= separation;
                        }
                    }
                }
                previous = vertex;
            }
        }

        private static IEnumerable<TracingSeed> RandomTracingSeedsInBounds(Func<double> random, int count, IFloat2Field major, IFloat2Field minor, Float2 min, Float2 max, Func<Float2, bool> isOutOfBounds)
        {
            var extent = (max - min);

            for (int i = 0; i < count; i++)
            {
                var p = new Float2((float)random() * extent.x, (float)random() * extent.y) + min;
                if (isOutOfBounds(p))
                    i--;
                else
                    yield return new TracingSeed(p, major, minor);
            }
        }

        private TracingSeed? RemoveTracingSeed(IMinHeap<KeyValuePair<float, TracingSeed>> seeds, BaseScalarField separation, float cosineSearchAngle, Func<TracerEdge, bool> edgeFilter = null)
        {
            while (seeds.Count > 0)
            {
                //Get the highest priority seed
                var s = seeds.RemoveMin().Value;

                //Check if it's valid
                var d = s.Field.Sample(s.Point);

                //Degenerate point?
                var l = d.Magnitude();
                if (l < 0.001f)
                    continue;

                var sep = separation.Sample(s.Point);

                //Normalize direction
                d /= l;

                //Get edges near this point and check if there is a parallel edge
                if (FindTracerEdges(s.Point, sep).Where(e => edgeFilter == null || edgeFilter(e)).Any(e => Math.Abs(Float2.Dot(e.Direction, d)) > cosineSearchAngle))
                    continue;

                return s;
            }

            //No valid seeds found
            return null;
        }

        private static void AddTracingSeed(IMinHeap<KeyValuePair<float, TracingSeed>> seeds, TracingSeed seed, BaseScalarField priority = null, BaseScalarField separation = null)
        {
            
            var prio = priority.SafeSample(seed.Point)
                     + 1 / separation.SafeSample(seed.Point);

            seeds.Add(new KeyValuePair<float, TracingSeed>(-prio, seed));
        }

        private IEnumerable<TraceVertex> Vertices()
        {
            return _vertices.Intersects(
                new Bounds2(
                    new Float2(float.MinValue),
                    new Float2(float.MaxValue)
                )
            ).Where(a => a.EdgeCount > 0);
        }
        #endregion
    }
}