using System;
using System.Linq;
using System.Linq.Expressions;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Wire.Modules
{
    public class Module
    {
        protected Module() { ID = 0; }

        internal Module(ulong id)
        {
            ID = id;
            Console.WriteLine(id);
        }

        public readonly ulong ID;

        private Action? UpdateMethod = null;

        internal static unsafe void* GetUpdateMethod(object obj)
        {
            MethodInfo? methodInfo = obj.GetType().GetMethod("OnUpdate", BindingFlags.Instance | BindingFlags.NonPublic);

            if (methodInfo == null)
            {
                return null;
            }

            (obj as Module)!.UpdateMethod = (Action)CreateDelegate(methodInfo, obj);

            delegate* unmanaged[Cdecl]<void> ptr = (delegate* unmanaged[Cdecl]<void>)Marshal.GetFunctionPointerForDelegate((obj as Module)!.UpdateMethod!);

            return ptr;
        }

        public static Delegate CreateDelegate(MethodInfo methodInfo, object target)
        {
            Func<Type[], Type> getType;
            var isAction = methodInfo.ReturnType.Equals((typeof(void)));
            var types = methodInfo.GetParameters().Select(p => p.ParameterType);

            if (isAction)
            {
                getType = Expression.GetActionType;
            }
            else
            {
                getType = Expression.GetFuncType;
                types = types.Concat(new[] { methodInfo.ReturnType });
            }

            if (methodInfo.IsStatic)
            {
                return Delegate.CreateDelegate(getType(types.ToArray()), methodInfo);
            }

            return Delegate.CreateDelegate(getType(types.ToArray()), target, methodInfo.Name);
        }
    }
}
