#pragma once

// 管理层的统一操作结果，GUI 根据返回值决定提示内容
enum class OpResult {
    Ok,
    InvalidInput,  // 输入为空或格式非法
    NotFound,      // 目标对象不存在
    TimeConflict,  // 预约时间与已有预约冲突
    InvalidState   // 当前状态不允许该操作
};
