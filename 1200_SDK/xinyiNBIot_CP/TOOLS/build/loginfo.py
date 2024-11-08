#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import argparse
# import time


class LogInfo:
    def __init__(self, proj_path, info_file, func_list, base_file, zero=False, replace=False, restore=False, version=str()):
        # 工程路径，该路径下的所有文件都会被替换
        self.root_path = None
        # loginfo 的路径，如果是生成loginfo，则该路径是生成的文件路径
        # 如果是根据恢复代码中被替换的字符串，则该路径是读取loginfo信息的文件路径
        self.info_path = None
        # 需要匹配的函数名称
        self.func_list = None
        # 记录基础的log文件，新生成的文件会基于此文件重新排列
        self.base_file = None
        # 是否需要清零log id的标志位
        self.reset_log_id = None
        # 是否需要更新log id的标志位
        self.refresh_log_id = None
        # 是否需要替换格式化字符串的标志位
        self.replace_format = None
        # 是否需要恢复格式化字符串的标志位
        self.restore_format = None
        # 是否需要生成 loginfo 文件
        self.generate_info = None
        # 保存版本号的字符串
        self.version_str = version
        # 用于保存 loginfo 格式化字符串的字典
        # 包含两个字典，分别为格式化字符串对log id的字典和log id对格式化字符串的字典
        # 以空间换时间，两个可以同时索引
        self.format_id_dict = dict()
        self.id_format_dict = dict()
        # 记录当前最大的log id，下一个log id从这个数值+1开始
        self.max_log_id = 0
        # 用于保存原语的字典
        self.primitive_dict = dict()
        # 检查参数配置
        self.__config_check_and_set(proj_path, info_file, func_list, base_file, zero, replace, restore)
        # 函数或是参数开头的无效字符，剔除掉这些字符，才认为是函数名称或是参数的开始
        self.invalid_char_list = [' ', '\t', '\r', '\n', '\\']
        # 从工程文件中，或是从 loginfo 中把有效的 log_id 和格式化字符串的键值对保存到字典中
        self.get_init_dict()

    def __config_check_and_set(self, proj_path, info_file, func_list, base_file, zero, replace, restore):
        """设置参数并检查参数合法性"""
        # 设置各个参数
        self.__set_root_path(proj_path)
        self.__set_info_path(info_file)
        self.__set_base_file(base_file)
        self.__set_func_list(func_list)
        self.__set_flags(zero, replace, restore)
        # 判断是否是恢复字符串，如果是恢复，则loginfo文件必须存在
        if self.restore_format:
            if (not os.path.exists(self.info_path)) or (not os.path.isfile(self.info_path)):
                raise ValueError('log info file must be exists when restore log string')
        # 否则就是替换log的id，当 self.replace_format 为True时还要替换log字符串，这时需要生成loginfo
        else:
            # 如果loginfo的保存路径不存在，需要创建该路径
            dir_name = os.path.dirname(self.info_path)
            # 递归创建目录
            if not os.path.exists(dir_name):
                os.makedirs(dir_name)

    def __set_root_path(self, proj_path):
        # 如果工程路径为空，则认为参数无效，退出脚本
        if proj_path is None:
            raise ValueError('project path must be a valid directory')
        # 获取工程的绝对路径
        project_path = os.path.abspath(proj_path)
        # 判断路径是否是目录，不是目录说明是无效参数
        if not os.path.isdir(project_path):
            raise ValueError('project path must be a valid directory')
        # 设置有效路径
        self.root_path = project_path

    def __set_info_path(self, info_path):
        # 获取文件的绝对路径
        self.info_path = os.path.abspath(info_path)

    def __set_func_list(self, func_list):
        # 匹配的函数列表不能为空
        if func_list is None:
            raise ValueError('the match function cannot be None')
        # 保存需要匹配的函数列表
        self.func_list = func_list

    def __set_base_file(self, base_file):
        # 匹配的函数列表不能为空
        if base_file is not None:
            # 获取绝对路径
            base_file = os.path.abspath(base_file)
            # 如果填了基础log文件的路径，但是此log文件不存在，抛出异常
            if (not os.path.exists(base_file)) or (not os.path.isfile(base_file)):
                raise ValueError('base log info file path is set but not exist')
        # 保存基础log文件的路径
        self.base_file = base_file

    def __set_flags(self, zero, replace, restore):
        # 判断入参是否是bool类型，为bool类型才认为参数正确
        if not isinstance(replace, bool):
            raise ValueError('input replace flag must be bool type')
        # 判断入参是否是bool类型，为bool类型才认为参数正确
        if not isinstance(restore, bool):
            raise ValueError('input restore flag must be bool type')
        # 判断入参是否是bool类型，为bool类型才认为参数正确
        if not isinstance(zero, bool):
            raise ValueError('input zero flag must be bool type')
        # 替换和恢复的标志位不能同时为True，但是可以同时为False
        if replace and restore:
            raise ValueError('cannot set replace flag and restore flag concurrently')
        # 清零选项不能和替换选项同时使用
        if zero and replace:
            raise ValueError('zero flag cannot set with replace flag')
        # 设置各个标志位
        self.reset_log_id = zero
        self.replace_format = replace
        self.restore_format = restore
        # 设置是否需要更新log id的标志位，以及是否生成 loginfo 文件
        # 目前更新 log id 就要生成 loginfo 文件
        if zero or restore:
            self.refresh_log_id = False
            self.generate_info = False
        else:
            self.refresh_log_id = True
            self.generate_info = True

    def __get_project_c_file_list(self):
        """获取工程目录下的c文件列表，用于遍历文件"""
        # 获取目标路径的文件列表
        file_list = list()
        for root, dirs, files in os.walk(self.root_path):
            for name in files:
                # 仅返回 c 文件的路径
                if name.endswith('.c'):
                    file_path = os.path.join(root, name)
                    file_list.append(file_path)
        return file_list

    def __get_primitive_file_list(self):
        """获取包含原语的文件列表"""
        file_list = list()
        for root, dirs, files in os.walk(self.root_path):
            # 文件路径包含 'diag_ctrl',才是有效路径
            if 'diag_ctrl' in root:
                for name in files:
                    # 仅扫描 .h 文件
                    if name.endswith('.h'):
                        file_path = os.path.join(root, name)
                        file_list.append(file_path)
        return file_list

    def __get_log_id_index(self, par_str):
        """
        从log id所在的字符串中获取填写的内容，之所以要获取，是因为填写的参数可能包含注释或者无效字符等，
        这个函数就是去除注释和无效字符，获取填写的有效内容。
        返回参数起始索引、结束索引、参数内容的列表：[start_index, end_index, log_id_str]
        """
        # 去除掉参数开头的这些字符，这些字符不是参数的实际有效参数
        invalid_char_list = self.invalid_char_list
        # 保存参数开始的索引，默认开始为0
        start_index = 0
        # 保存参数字符串长度，后面会经常用到
        par_str_len = len(par_str)
        # 找到参数开始的索引
        # 死循环去除所有的无效字符和注释
        while True:
            # 去除无效字符
            for index in range(start_index, par_str_len):
                if par_str[index] not in invalid_char_list:
                    start_index = index
                    break
            # 标识当前是否有注释的标志位
            # 如果以 '/*' 开始，则flag值为1，如果以 '//' 开始，则flag值为2，否则值为3
            comment_flag = 1 if par_str[start_index:].startswith('/*') else 2 if par_str[start_index:].startswith('//') else 0
            # 以 '/*' 开始，则寻找 '*/'
            if comment_flag == 1:
                for index in range(start_index+2, par_str_len):
                    if par_str[index:].startswith('*/'):
                        start_index = index + 2
                        break
            # 以 '//' 开始，则寻找 '\n'
            elif comment_flag == 2:
                for index in range(start_index+2, par_str_len):
                    if par_str[index] == '\n':
                        start_index = index + 1
                        break
            # 没有注释，则退出循环，找到起始索引
            else:
                break
        # 保存结束的索引
        end_index = start_index + 1
        # 从此索引开始寻找结束索引，结束为参数长度，这是数值会在循环中匹配更新
        # 如果遇到无效字符或注释，则认为参数内容结束
        while end_index < par_str_len:
            # 遇到无效字符，参数内容结束
            if par_str[end_index] in invalid_char_list:
                break
            # 遇到 '/*' 注释，参数内容结束
            if par_str[end_index:].startswith('/*'):
                break
            # 遇到 '//' 注释，参数内容结束
            if par_str[end_index:].startswith('//'):
                break
            # 更新结束索引
            end_index += 1
        # 获取传入的字符串中，有效的参数部分，并加上双引号
        log_id_str = par_str[start_index:end_index]
        # 返回结束和开始的列表
        return [start_index, end_index, log_id_str]

    def __get_format_str_index(self, par_str):
        """
        获取函数有效参数的索引，包含参数开始和结束的索引，传入的参数可能仍然包含参数分隔符 ','，但只可能存在格式化字符串中
        当传入的参数是格式化字符串时，传入的参数必须包含双引号 "，返回的字符串也包含双引号 "
        返回参数起始索引、结束索引、参数内容的列表：[start_index, end_index, format_str]
        """
        # 去除掉参数开头的这些字符，这些字符不是参数的实际有效参数
        invalid_char_list = self.invalid_char_list
        # 保存参数开始和结束的索引，默认开始为0，结束为参数长度，这是数值会在循环中匹配更新
        start_index = 0
        end_index = 0
        # 保存参数字符串长度，后面会经常用到
        par_str_len = len(par_str)
        # 找到参数开始的索引
        # 死循环去除所有的无效字符和注释
        while True:
            # 去除无效字符
            for index in range(start_index, par_str_len):
                if par_str[index] not in invalid_char_list:
                    start_index = index
                    break
            # 标识当前是否有注释的标志位
            # 如果以 '/*' 开始，则flag值为1，如果以 '//' 开始，则flag值为2，否则值为3
            comment_flag = 1 if par_str[start_index:].startswith('/*') else 2 if par_str[start_index:].startswith('//') else 0
            # 以 '/*' 开始，则寻找 '*/'
            if comment_flag == 1:
                for index in range(start_index+2, par_str_len):
                    if par_str[index:].startswith('*/'):
                        start_index = index + 2
                        break
            # 以 '//' 开始，则寻找 '\n'
            elif comment_flag == 2:
                for index in range(start_index+2, par_str_len):
                    if par_str[index] == '\n':
                        start_index = index + 1
                        break
            # 没有注释，则退出循环，找到起始索引
            else:
                break
        # TODO: 如果这个字符不是双引号，除了是宏之外，其他情况必然存在语法错误，当前宏字符串还无法处理，故报错
        if par_str[start_index] != '"':
            raise ValueError(f'The formatting string has a syntax error: {par_str}')
        # 用于保存实际的格式化字符串
        format_str = str()
        # 找到参数结束的索引
        # 如果参数以 " 开头，则认为这个参数是一个字符串，找到最后一个 " 才认为该参数结束
        if par_str[start_index] == '"':
            # 引号标志，标记当前内容是否在字符串内
            quotes_flag = True
            # 上一个引号的索引，用来切片字符串，获得格式化字符串的内容
            last_quote_index = start_index
            # 包含了对多个字符串拼接成的格式化字符串的处理，如 "test1" "test2" 这样的实际是 "test1test2"
            # 记录当前遍历的索引值
            index = start_index + 1
            while index < par_str_len:
                # 仅当该字符串为 "，且前一个字符串不为 '\\' 时，才认为该字符串结束
                if (par_str[index] == '"') and (par_str[index - 1] != '\\'):
                    # 更新结束的索引，这里加一是为了字符串切片时不包含右边界
                    end_index = index + 1
                    # 引号标志取反
                    quotes_flag = not quotes_flag
                    # 如果当前引号标志为False，认为这里是一段引号的内容，取出其中内容
                    if quotes_flag is False:
                        # 这个范围内的字符串时格式化字符串的内容
                        format_str += par_str[last_quote_index+1:index]
                    else:
                        # 引号计数为奇数时，记录起始索引，在取格式化字符串内容时会用到
                        last_quote_index = index
                    # 更新索引，进行下一次循环
                    index += 1
                # 那么如果后面的字符不在无效字符列表中，就说明格式化字符串结束，或者有语法错误，这些情况都结束继续寻找
                # 当前已经有成对的双引号出现，但是中间存在无效字符或是注释，则再次执行跳过无效字符或注释的操作
                elif quotes_flag is False:
                    # 当前是否有注释的标志位
                    # 1: 有 /* 类型的注释
                    # 2: 有 // 类型的注释
                    comment_flag = 0
                    # 记录当前遍历的索引值
                    tmp_index = end_index
                    # 循环去除所有的无效字符和注释
                    while tmp_index < par_str_len:
                        # 跳过无效字符
                        if (comment_flag == 0) and (par_str[tmp_index] in invalid_char_list):
                            tmp_index += 1
                            index = tmp_index
                            continue
                        else:
                            # 如果当前没有注释，查看接下来的数据是否是注释
                            if comment_flag == 0:
                                # 标识当前是否有注释的标志位
                                # 如果以 '/*' 开始，则flag值为1，如果以 '//' 开始，则flag值为2，否则值为3
                                comment_flag = 1 if par_str[tmp_index:].startswith('/*') else 2 if par_str[tmp_index:].startswith('//') else 0
                                # 除了正常循环结束外，只有这一处循环退出点
                                # 现在此字符既不是无效字符，又不在注释之内，说明是有效字符，退出循环，继续提取格式化字符串
                                if comment_flag == 0:
                                    # 如果是这两个字符，则认为格式化字符串已经结束，将索引设置成参数字符串长度，以退出主循环
                                    if par_str[tmp_index] == ',' or par_str[tmp_index] == ')':
                                        index = par_str_len
                                    # 如果这个字符不是双引号，可能是宏定义的字符串，当前宏字符串还无法处理，故报错
                                    elif par_str[tmp_index] != '"':
                                        index = par_str_len
                                        raise ValueError('The formatting string has a syntax error: {par_str}')
                                    # 更新新的索引，从此处继续提取格式化字符串
                                    else:
                                        index = tmp_index
                                    # 退出循环，继续检测
                                    break
                                else:
                                    tmp_index += 1
                            # 当前有 /* 类型的注释
                            elif comment_flag == 1:
                                # 找到注释尾 '*/'
                                if par_str[tmp_index:].startswith('*/'):
                                    comment_flag = 0
                                    tmp_index += 2
                                else:
                                    tmp_index += 1
                            # 当前有 // 类型的注释
                            elif comment_flag == 2:
                                # 找到注释尾 '\n'
                                if par_str[tmp_index] == '\n':
                                    comment_flag = 0
                                    tmp_index += 1
                                else:
                                    tmp_index += 1
                # 更新索引
                else:
                    index += 1
        format_str = '"' + format_str + '"'
        # 返回结束和开始的列表
        return [start_index, end_index, format_str]

    def __get_new_log_id(self, log_id_str, format_str):
        """如果 id 不需要更新，则返回 None，否则返回一个未被占用的 log id，注意传进来的 format_str 带有双引号"""
        # noinspection PyBroadException
        try:
            # 尝试以10进制字符串解析传入的 log id
            log_id = int(log_id_str, 10)
        except Exception:
            log_id = 0
        else:
            pass
        finally:
            pass
        # 去除字符串中的 双引号
        format_str = format_str[1:len(format_str)-1]
        # 如果此字符串已经在字典中，则取出字典中的log id
        if format_str in self.format_id_dict:
            new_log_id = self.format_id_dict[format_str]
            # 如果当前log id与字典中的log id相同，则认为无需获取新的log id
            if new_log_id == log_id:
                new_log_id = None
        # 否则获取一个新的log id与格式化字符串做匹配
        else:
            # 从最大值获取最新的log id，并更新最大值
            new_log_id = self.max_log_id + 1
            self.max_log_id += 1
        # 返回新的 log id
        return new_log_id

    @staticmethod
    def __get_reset_log_id(log_id_str):
        """如果 id 未更新，则返回 None"""
        # noinspection PyBroadException
        try:
            # 尝试以10进制字符串解析传入的 log id
            log_id = int(log_id_str, 10)
        except Exception:
            log_id = 1
        else:
            pass
        finally:
            pass
        # 判断当前的 log_id 是否为0，如果如果为0，则不需要更新 log id
        if log_id != 0:
            new_log_id = 0
        else:
            new_log_id = None
        # 返回新的 log id
        return new_log_id

    @staticmethod
    def __get_replace_format(format_str, file_path):
        """
        获取简化的字符串，简化后只保留格式化部分，注意传进来的格式化字符串包含双引号
        传入的文件路径只是为了出错时的打印
        """
        # 这里可能会出现访问字符串越界的错误，所以加上try
        # noinspection PyBroadException
        try:
            # 去除双引号的临时格式化字符串
            format_str = format_str[1:len(format_str)-1]
            # 用于保存简化后的格式化字符串
            new_format_str = str()
            # 去除双引号后格式化字符串的长度
            format_len = len(format_str)
            # 记录当前遍历格式化字符串的索引
            index = 0
            # 循环开始遍历，参考C库格式化代码的方式，参考函数为 vsnprintf
            while index < format_len:
                # 格式说明符，format specifier
                if format_str[index] != '%':
                    index += 1
                    continue
                # 记录格式化开始的索引，用于获取实际有效的格式化字符
                start_index = index
                index += 1
                # 评估标志位，evaluate flags
                # 继续跳过下列字符，这些是格式化字符串的有效符号
                while True:
                    if format_str[index] in ['0', '-', '+', ' ', '#']:
                        index += 1
                    else:
                        break
                # 评估字段长度，evaluate length field
                # 跳过所有数字部分，这部分也是有效符号
                if format_str[index].isdigit():
                    index += 1
                    while format_str[index].isdigit():
                        index += 1
                # 如果有 '*'，这也是有效符号
                elif format_str[index] == '*':
                    index += 1
                # 精度评估领域，evaluate precision field
                # 跳过小数点以及后面的数字部分，这部分也是有效符号
                if format_str[index] == '.':
                    index += 1
                    if format_str[index].isdigit():
                        index += 1
                        while format_str[index].isdigit():
                            index += 1
                    # 如果有 '*'，这也是有效符号
                    elif format_str[index] == '*':
                        index += 1
                # 评估字段长度，evaluate length field
                if format_str[index] == 'l':
                    index += 1
                    if format_str[index] == 'l':
                        index += 1
                elif format_str[index] == 'h':
                    index += 1
                    if format_str[index] == 'h':
                        index += 1
                elif format_str[index] in ['t', 'j', 'z']:
                    index += 1
                # 评估说明符，evaluate specifier
                specifier_char_list = ['d', 'i', 'u', 'x', 'X', 'o', 'b', 'f', 'F', 'e', 'E', 'g', 'G', 'c', 's', 'p', '%']
                if format_str[index] in specifier_char_list:
                    index += 1
                # else:
                #     index += 1
                # 保存实际有效的格式化字符
                new_format_str += format_str[start_index:index]
            # 如果格式化字符串提取其中有效匹配符后，和原来字符串相等，说明格式化字符串没有多余的字符，可以返回有效字符串
            if new_format_str != format_str:
                new_format_str = '"' + new_format_str + '"'
                return new_format_str
        # 出错不作任何处理，返回None
        except Exception:
            print(f'warning: Format string match failed in {file_path}, and format string is "{format_str}"')
        else:
            pass
        finally:
            pass
        # 其他情况都要返回 None
        return None

    def __get_restore_format(self, log_id_str, format_str):
        """获取指定 log id 对应的格式化字符串，该函数在恢复格式化字符串的时候会被调用到"""
        # noinspection PyBroadException
        try:
            # 尝试以10进制字符串解析传入的 log id
            log_id = int(log_id_str, 10)
        except Exception:
            pass
        else:
            # log id 是十进制字符串，且该 id 在字典中，则返回对应的格式化字符串
            # 恢复格式化字符串时，这个字典是从 loginfo 中得到的键值对
            if log_id in self.id_format_dict:
                # 获取到格式化字符串
                new_format_str = self.id_format_dict[log_id]
                # loginfo 中的格式化字符串不包含双引号，需要加上
                new_format_str = '"' + new_format_str + '"'
                # 仅当字符串需要更改的时候才需要替换
                if new_format_str != format_str:
                    # 返回要替换的格式化字符串，其他情况都返回 None
                    return new_format_str
        finally:
            pass
        # 参数不合法或者此字符串无需替换，则返回 None
        return None

    def __try_add_to_dict(self, log_id, format_str):
        # format_str 传入的是包含首尾的 "，需要去除 "
        # 去除首尾的 "，获取格式化字符串内容
        format_str = format_str[1:len(format_str)-1]
        # 当前格式化字符串不在字典中时，才添加到字典
        if format_str not in self.format_id_dict:
            self.format_id_dict[format_str] = log_id
            self.id_format_dict[log_id] = format_str

    def __add_string_primitive_dict(self, content):
        """这个函数从从字符串中找到原语结构体，把相关信息保存到字典"""
        # 找到 'MSG_SHOW_NAME' 关键字，以这种方式定义的原语才认为有效
        primitive_list = content.split('MSG_SHOW_NAME')
        # 去除掉参数开头的这些字符，这些字符不是参数的实际有效参数
        strip_str = ''.join(self.invalid_char_list)
        # 拆分的列表长度大于1时，才会进行循环，才认为找到了 'MSG_SHOW_NAME' 并做了拆分
        for index in range(1, len(primitive_list)):
            # 当前原语字符串
            cur_primitive = primitive_list[index]
            # 找到左右括号的索引
            left_bracket = cur_primitive.find('(')
            right_bracket = cur_primitive.find(')')
            # 没有直到左右小括号则认为这个原语无效，继续循环
            if left_bracket == -1 or right_bracket == -1:
                continue
            # 获取原语字符串，不包含左右括号
            primitive_str = cur_primitive[left_bracket+1:right_bracket]
            # 使用 ',' 拆分字符串
            struct_list = primitive_str.split(',')
            # 列表长度必须是2，认为只有两个成员才是正常的
            if len(struct_list) == 2:
                # 获取名称和结构体，去除无效字符串
                class_id = struct_list[0].strip(strip_str)
                struct_name = struct_list[1].strip(strip_str)
                # 如果字典中没有同名的原语键值对，则加入字典
                if class_id not in self.primitive_dict:
                    self.primitive_dict[class_id] = struct_name
                # 字典中有同名的键，如果键值不一样，则覆盖掉原来的键值，并打印提示信息
                else:
                    old_struct_name = self.primitive_dict[class_id]
                    # 当前的键值和已经存在的键值不一样
                    if old_struct_name != struct_name:
                        # 先删除原有的的原语项，使新的键值对在字典的最后
                        self.primitive_dict.pop(class_id)
                        self.primitive_dict[class_id] = struct_name
                        print('warning: duplicate key value, primitive class id is {class_id}, old struct name is {old_struct_name}')

    def __add_file_primitive_dict(self, file_path):
        """这个函数从从文件中读取内容，找到原语结构体，把相关信息保存到字典"""
        # 读取文件所有内容
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f_in:
            file_content = f_in.read()
            f_in.close()
        # 找到 'MSG_SHOW_NAME' 关键字，以这种方式定义的原语才认为有效
        # 这个关键字在头文件中宏定义实现，但是统计时，宏定义不能统计进去，认为只有在枚举中才是有效值
        if 'MSG_SHOW_NAME' not in file_content:
            return
        # 开始遍历的索引，从0开始
        start_index = 0
        # 死循环开始寻找
        while True:
            # 从索引处开始寻找 'enum'
            enum_index = file_content.find('enum', start_index)
            # 没有找到关键字时，直接退出
            if enum_index == -1:
                break
            # 找到 'enum' 之后，开始找左右大括号，为了获取大括号中的内容
            left_brace = file_content.find('{', enum_index)
            right_brace = file_content.find('}', enum_index)
            # 必须左右大括号都找到，才能认为找到了正确的枚举值
            if left_brace == -1 or right_brace == -1:
                break
            # 把右大括号的索引赋值给起始索引，下次循环继续匹配
            start_index = right_brace
            # 获取枚举花括号中的内容
            enum_content = file_content[left_brace+1:right_brace]
            self.__add_string_primitive_dict(enum_content)

    def __read_log_info_dict(self, log_path):
        # 读取文件所有内容
        with open(log_path, 'r', encoding='utf-8', errors='ignore') as f_in:
            file_content = f_in.read()
            f_in.close()
        # 记录最大的log id
        max_log_id = 0
        # 以 '\n' 为分隔符，分隔字符串为列表
        line_list = file_content.splitlines(keepends=False)
        # 循环遍历列表，把当前内容添加到字典
        for cur_line in line_list:
            # 以 '$I:' 开头的行是有 log id 和格式化字符串的键值对的行
            if cur_line.startswith('$I:'):
                # 以 ':' 为分隔符分隔字符串，最多分隔3个 ':'
                # 行的内容为：$I:log_id:S:format_str，所以只能切分三个
                tmp_list = cur_line.split(':', maxsplit=3)
                # 获取 log_id 和格式化字符串
                log_id = int(tmp_list[1], 10)
                format_str = tmp_list[3]
                # 加入到字典，两个字典，用于通过格式化字符串和log id都能够快速索引到对方
                self.format_id_dict[format_str] = log_id
                self.id_format_dict[log_id] = format_str
                # 认为log id是递增的，下一个log id总比前一个大
                max_log_id = log_id
            # 以 '&' 开头的行是有class_id和解码结构体的键值对的行
            elif cur_line.startswith('&'):
                # 以 ',' 为分隔符分隔字符串，能且只能分割成2个子串，
                # 行的内容为：&class_id,struct_name
                tmp_list = cur_line.split(',')
                # 获取 class_id 和结构体名称
                class_id = tmp_list[0][1:].strip()
                struct_name = tmp_list[1].strip()
                # 加入到字典
                self.primitive_dict[class_id] = struct_name
        # 记录最大的log id
        self.max_log_id = max_log_id

    def __read_primitive_dict(self):
        # 获取包含原语结构体的头文件列表
        primitive_list = self.__get_primitive_file_list()
        # 遍历每个文件，读取原语的结构体信息并保存到字典
        for cur_file in primitive_list:
            self.__add_file_primitive_dict(cur_file)

    def get_init_dict(self):
        """生成对象是调用该函数，用来获取初始的字典键值对，需要判断从文件中读取还是从loginfo中读取"""
        # 判断是否是恢复字符串，如果是恢复，则需要从loginfo中读取键值对
        if self.restore_format:
            self.__read_log_info_dict(self.info_path)
        # 如果有基础的log文件，则先从基础的log文件中获取键值对
        elif self.base_file is not None:
            self.__read_log_info_dict(self.base_file)

    def modify_log_parameter(self, par_str, file_path):
        """
        注意传进来的参数包含左右括号
        如果字符串需要修改，则返回修改后的字符串，否则返回None
        传入的文件路径只是为了出错时的打印
        """
        # 字符串是否被修改修改的标志位，如果参数被修改了，则需要置这个flag位
        modify_flag = False
        # 是否需要把 log id 和格式化字符串加入到字典的标志位，
        add_dict_flag = False
        # 用于保存新的 log id 和格式化字符串
        new_log_id = None
        new_format_str = None
        # 去除参数首尾的括号，传入参数时保证了首尾是左右括号
        par_str = par_str[1:len(par_str)-1]
        # 以 ',' 为分隔符，分隔参数字符串，找到实际参数字符串，然后作修改
        # 最多分隔3个','，获得4个子串
        par_list = par_str.split(',', maxsplit=3)
        # 获取log id和格式化字符串的参数
        # TODO: 这里认为当前的参数最少4个，当前的log接口参数是这样的
        par_log_id = par_list[0]
        par_format = par_list[3]
        # 获取实际参数的起始和结束索引
        index_list = self.__get_log_id_index(par_log_id)
        # 获得实际有效的 log id 字符串以及字符串的起始索引和结束索引
        id_start = index_list[0]
        id_end = index_list[1]
        log_id = index_list[2]
        # 获取实际参数的起始和结束索引
        index_list = self.__get_format_str_index(par_format)
        # 获得实际有效的格式化字符串以及字符串的起始索引和结束索引
        format_start = index_list[0]
        format_end = index_list[1]
        format_str = index_list[2]
        # 如果需要更新或清零 log id
        if self.refresh_log_id:
            new_log_id = self.__get_new_log_id(log_id, format_str)
            # 只要更新了 log id，就应该要生成 loginfo，不存在其他条件
            add_dict_flag = True
        elif self.reset_log_id:
            # 判断当前的 log id 是否需要清零，如果原来就是0，则不需要清零
            new_log_id = self.__get_reset_log_id(log_id)
        # 如果没有获取到新的 log id，则认为当前的 log id 不需要更新，获取到实际的 id
        if new_log_id is None:
            new_log_id = int(log_id, 10)
        # 否则需要更新，则修改列表成员的值
        else:
            # 获得更新后的 log id 字符串，并修改参数字符串
            par_list[0] = par_log_id[0:id_start] + str(new_log_id) + par_log_id[id_end:]
            # 设置需要更新参数字符串的标志
            modify_flag = True
        # 如果需要替换或恢复格式化字符串
        if self.replace_format:
            # 传入的文件路径只是为了出错时的打印
            new_format_str = self.__get_replace_format(format_str, file_path)
        elif self.restore_format:
            new_format_str = self.__get_restore_format(log_id, format_str)
        # 如果没有获取到更新后的格式化字符串，则认为当前的格式化字符串不需要更新，获取到实际的格式化字符串
        # 否则需要更新，则修改列表成员的值
        if new_format_str is not None:
            # 获得更新后的 log id 字符串，并修改参数字符串
            par_list[3] = par_format[0:format_start] + new_format_str + par_format[format_end:]
            # 设置需要更新参数字符串的标志
            modify_flag = True
        # 把当前的 log id 和格式化字符串保存到字典中，格式化字符串需要保存原始的字符串
        if add_dict_flag:
            self.__try_add_to_dict(new_log_id, format_str)
        # 如果参数需要修改，额返回修改后的字符串
        if modify_flag:
            # 得到修改过的字符串
            par_str = ','.join(par_list)
            par_str = '(' + par_str + ')'
            # 返回修改过的字符串
            return par_str
        # 参数不需要修改，返回 None 即可
        return None

    def modify_file(self, file_path):
        # 读取文件所有内容
        with open(file_path, 'r', encoding='utf-8', errors='ignore') as f_in:
            file_content = f_in.read()
            f_in.close()
        # 用来标记是否对文件做了修改，如果修改了，则需要把修改写入到文件，否则无需写入
        # 如果不判断直接写入的话，每个文件都被更新了，即使文件内容没有改变，这样会导致增量编译时每个文件都是最新的
        modify_flag = 0
        # 字符串开头需要去除的字符
        lstrip_list = ''.join(self.invalid_char_list)
        # 循环遍历每个函数符号，这些需要被替换
        for cur_func in self.func_list:
            # 以函数名为分隔符，分隔整个文件，获得调用该函数的点
            valid_list = file_content.split(cur_func)
            # 当前函数的调用点是否有被修改过的标志位，这个标志位用于判断每次循环后是否需要更新 file_content
            func_modify_flag = 0
            # 开始遍历每个函数调用点，第一个不是有效参数
            for index in range(1, len(valid_list)):
                # 获取当前调用的参数字符串
                valid_str = valid_list[index]
                # 去除字符串左边的 空格(' ')、制表符('\t')、换行('\r' '\n')、C语言换行符('\\')
                # 然后还以 '(' 开头的话，则认为这是一个有效的函数调用
                # TODO: 如果注释中也有这样的函数，该如何排除这样的情况，暂时的处理方法是注释也当成正常代码处理，会去替换恢复
                # 去除字符串开头所有的指定字符
                lstrip_str = valid_str.lstrip(lstrip_list)
                # 如果这个时候的字符串还以 '(' 开头，则认为这是一个正确的函数调用
                if lstrip_str.startswith('('):
                    # 获取函数参数左括号在字符串中的位置
                    left_bracket_index = 0
                    while valid_str[left_bracket_index] != '(':
                        left_bracket_index += 1
                    # 获取函数参数右括号在字符串中的位置
                    right_bracket_index = left_bracket_index + 1
                    # 括号对应标志，找到函数调用的左括号对应的右括号的位置
                    bracket_zero_flag = 1
                    # 引号标志，标记当前内容在字符串内，不关注字符串的内容
                    quotes_flag = False
                    # 死循环等待找到对应函数参数结束
                    while bracket_zero_flag != 0:
                        # 如果当前字符为双引号 '"'，且前一个字符不是转义字符 '\\'，则认为这个是一个字符串，不关注字符串的内容
                        # 这里直接使用 "right_bracket_index - 1"，因为这里不可能溢出
                        if (valid_str[right_bracket_index] == '"') and (valid_str[right_bracket_index - 1] != '\\'):
                            # 如果当前的引号为字符串的开始或结束，则对引号标志取反，标志当前的内容是否在字符串内
                            quotes_flag = not quotes_flag
                        # 只有不是字符串中的内容，才进行括号的判断
                        if quotes_flag is False:
                            # '(' 加一，')' 减一，直到这个变量减为0，就找到了函数开头 '(' 对应的 ')' 位置
                            if valid_str[right_bracket_index] == '(':
                                bracket_zero_flag += 1
                            elif valid_str[right_bracket_index] == ')':
                                bracket_zero_flag -= 1
                        # 先判断再加一，最红得到的right_bracket_index的值是目标 ')' 的索引加一，方便使用切片
                        right_bracket_index += 1
                    # 获取到实际函数参数的字符串，这个字符串包含左右括号
                    par_str_with_bracket = valid_str[left_bracket_index:right_bracket_index]
                    # 以 ',' 拆分参数字符串，获取每个参数的内容
                    par_list = par_str_with_bracket.split(',')
                    # TODO: 这里最少分出4个参数，由于当前函数需求，少于这些参数则认为参数过少，后续函数修改，这里可能需要修改
                    if len(par_list) < 4:
                        # 参数过少时，说明这不是一个有效的调用，丢弃该次匹配
                        continue
                    # 修改参数字符串，传入文件路径只是为了出错时打印
                    modify_str = self.modify_log_parameter(par_str_with_bracket, file_path)
                    # 修改参数字符串后，如果返回None，则表示无需修改，否则返回修改后的字符串
                    if modify_str is not None:
                        # 设置需要修改文件的标志位，会在循环结束后修改文件
                        func_modify_flag += 1
                        # 重新拼接修改后的字符串，再把这个字符串赋值给拆分的列表，会用来更新 file_content
                        valid_str = valid_str[0:left_bracket_index] + modify_str + valid_str[right_bracket_index:]
                        valid_list[index] = valid_str
            # 如果文件已经被修改过，需要更新文件内容，暂时不写入文件，最后统一写入文件
            if func_modify_flag != 0:
                # 更新 file_content，下一次循环在此基础上继续修改
                file_content = cur_func.join(valid_list)
                # 如果文件已经有更新，则最后肯定要回写文件
                modify_flag += func_modify_flag
        # 根据 modify_flag 判断是否需要写入文件
        if modify_flag != 0:
            # 获取文件当前的访问时间，修改时间
            access_time = os.path.getatime(file_path)
            modify_time = os.path.getmtime(file_path)
            # 直接覆盖写入
            with open(file_path, 'w', encoding='utf-8', errors='ignore') as f_out:
                f_out.write(file_content)
                f_out.close()
            # 把文件访问时间和修改时间修改为原始时间的1秒后
            access_time += 0.001
            modify_time += 0.001
            # 更新源文件(.c)时间，这样可以让在文本编辑器中已经打开的源文件自动刷新
            os.utime(file_path, (access_time, modify_time))

    def generate_log_info(self):
        # 对字典键值排序
        log_info_list = sorted(self.id_format_dict.items(), key=lambda x: x[0], reverse=False)
        # 用于保存要写入 log 文件的字符串
        file_content = 'softVer:' + self.version_str + '\n'
        # 循环获取每个键值对，对应其每个要写入文件的字符串
        for cur_tuple in log_info_list:
            log_id = cur_tuple[0]
            format_str = cur_tuple[1]
            file_content += '$I:' + str(log_id) + ':S:' + format_str + '\n'
        # 获取原语的字典
        self.__read_primitive_dict()
        # 循环写入每个原语的字符串
        for cur_name in self.primitive_dict:
            primitive_struct = self.primitive_dict[cur_name]
            file_content += '&' + cur_name + ',' + primitive_struct + '\n'
        # 写入文件
        with open(self.info_path, 'w', encoding='utf_8', errors='ignore') as f_out:
            f_out.write(file_content)
            f_out.close()

    def modify(self):
        # 获取所有的 c 文件
        c_file_list = self.__get_project_c_file_list()
        # 循环更新每一个 c 文件
        for cur_file in c_file_list:
            self.modify_file(cur_file)
        # 如果需要生成 loginfo
        if self.generate_info:
            self.generate_log_info()


def set_argument(par_list=None):
    # 生成对象，并添加对脚本的描述
    # description: 在参数帮助文档之前显示的文本（默认值：无）
    # prog: 程序的名称，默认就是 sys.arg[0]
    # '%(prog)s': 这个字符串在帮助消息里表示的是程序名称
    description = '%(prog)s is use for generate log info or restore log string'
    parser = argparse.ArgumentParser(description=description)
    # name or flags - 一个命名或者一个选项字符串的列表，例如 foo 或 -f, --foo。
    # action - 当参数在命令行中出现时使用的动作基本类型。
    # nargs - 命令行参数应当消耗的数目。
    # const - 被一些 action 和 nargs 选择所需求的常数。
    # default - 当参数未在命令行中出现并且也不存在于命名空间对象时所产生的值。
    # type - 命令行参数应当被转换成的类型。
    # choices - 可用的参数的容器。
    # required - 此命令行选项是否可省略 （仅选项可用）。
    # help - 一个此选项作用的简单描述。
    # metavar - 在使用方法消息中使用的参数值示例。
    # dest - 被添加到 parse_args() 所返回对象上的属性名。
    # 增加设置工程路径的参数
    # action='store_true' 用来指示这个选项没有参数
    # required=True 指示该选项必须存在
    # metavar='<path>' 指示该参数的使用示例
    help_str = 'set project root path, script will modify the log string'
    parser.add_argument('-p', '--path', type=str, default=None, required=True, metavar='<path>', help=help_str)
    # 增加loginfo文件路径的参数
    help_str = 'set info file path, generate loginfo or restore log string'
    parser.add_argument('-i', '--info', type=str, default=None, required=True, metavar='<file>', help=help_str)
    # 增加需要匹配的函数的参数
    help_str = 'set match function list'
    parser.add_argument('-m', '--match', type=str, default=None, required=True, metavar='[func]', nargs='+', help=help_str)
    # 增加替换成log id的起始数值
    help_str = 'set base log file'
    parser.add_argument('-b', '--base', type=str, default=None, required=False, metavar='<file>', help=help_str)
    # 增加选择是否替换rodata字符的参数
    help_str = 'replace rodata string to save memory'
    parser.add_argument('-rp', '--replace', action='store_true', default=False, help=help_str)
    # 增加选择是否恢复字符串的参数
    help_str = 'restore rodata string'
    parser.add_argument('-rs', '--restore', action='store_true', default=False, help=help_str)
    # 增加把所有log的id设置成0的参数
    help_str = 'set all log id to zero'
    parser.add_argument('-z', '--zero', action='store_true', default=False, help=help_str)
    # 增加当前代码工程的版本号
    help_str = 'set project version'
    parser.add_argument('-v', '--version', type=str, default=str(), required=False, metavar='<string>', help=help_str)

    # 根据参数，传入参数列表，如果没有手动传入参数，则从命令行参数列表中获取
    args = parser.parse_args(par_list)
    # 返回参数列表
    # print(args)
    return args


def main():
    # 从命令行中获取参数
    par_list = None
    # 手动设置参数，调试用
    # par_str = r'-p E:/eclipse-workspace/4100_cp/ -i loginfo.info -b base_loginfo.info -m PrintLog PhyPrint0'
    # par_list = par_str.split()
    # 开始的时间点，用于计算耗时
    # start_time = time.perf_counter()
    # 传入所有输入的参数
    args = set_argument(par_list)
    # 生成对象，传入参数，并修改调用log的代码
    loginfo = LogInfo(args.path, args.info, args.match, args.base, args.zero, args.replace, args.restore, args.version)
    loginfo.modify()
    # 结束的时间点，用于计算耗时
    # end_time = time.perf_counter()
    # print(f'{end_time - start_time:.3f}s taken!')


if __name__ == "__main__":
    main()
