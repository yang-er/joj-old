﻿namespace JudgeCore
{
    /// <summary>
    /// 测试结果信息
    /// </summary>
    public struct TestInfo
    {
        /// <summary>
        /// 运行时间
        /// </summary>
        public double Time;

        /// <summary>
        /// 运行内存大小
        /// </summary>
        public long Memory;

        /// <summary>
        /// 运行结果
        /// </summary>
        public JudgeResult Result;
    }
}