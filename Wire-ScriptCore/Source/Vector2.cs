using System;
using System.Linq;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Runtime.CompilerServices;

namespace Wire
{
    public struct Vector2
    {
        public float X = 0, Y = 0;

        public float this[int index]
        {
            get
            {
                if (index == 0)
                {
                    return X;
                }
                else if (index == 1)
                {
                    return Y;
                }
                throw new IndexOutOfRangeException();
            }
            set
            {
                if (index == 0)
                {
                    X = value;
                }
                if (index == 1)
                {
                    Y = value;
                }
                throw new IndexOutOfRangeException();
            }
        }

        public Vector2(float x, float y)
        {
            X = x;
            Y = y;
        }

        public Vector2(float scalar)
        {
            X = scalar;
            Y = scalar;
        }

        public static Vector2 operator+(Vector2 a, Vector2 b)
        {
            return new Vector2(a.X + b.X, a.Y + b.Y);
        }

        public static Vector2 operator*(Vector2 vector, float scalar)
        {
            return new Vector2(vector.X * scalar, vector.Y * scalar);
        }

        public static Vector2 operator-(Vector2 a, Vector2 b)
        {
            return new Vector2(a.X - b.X, a.Y - b.Y);
        }

        public static Vector2 Lerp(Vector2 a, Vector2 b, float t)
        {
            return new Vector2(a.X + (b.X - a.X) * t, a.Y + (b.Y - a.Y) * t);
        }

        public override readonly string ToString()
        {
            return $"{X}, {Y}";
        }
    }
}
