/*
 Navicat MySQL Data Transfer

 Source Server         : localhost
 Source Server Type    : MySQL
 Source Server Version : 50731 (5.7.31-log)
 Source Host           : localhost:3306
 Source Schema         : 11

 Target Server Type    : MySQL
 Target Server Version : 50731 (5.7.31-log)
 File Encoding         : 65001

 Date: 28/04/2026 16:29:16
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for tb_processdetail
-- ----------------------------
DROP TABLE IF EXISTS `tb_processdetail`;
CREATE TABLE `tb_processdetail`  (
  `id` int(11) NOT NULL AUTO_INCREMENT COMMENT '自增主键',
  `OPT_INDEX` int(11) NOT NULL COMMENT '配置序号（每个场景从0开始）',
  `PROCESS_ID` int(11) NOT NULL COMMENT '关联场景ID',
  `OPT_CONTENT` json NOT NULL COMMENT '配置内容JSON',
  `OPT_NAME` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '配置名称',
  `OPT_TYPE` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '配置大类：信号源控制/高轨定位设备/流程逻辑控制',
  `created_at` datetime NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`id`) USING BTREE,
  UNIQUE INDEX `uk_process_detail`(`OPT_INDEX`, `PROCESS_ID`) USING BTREE,
  INDEX `idx_process_id`(`PROCESS_ID`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 2199 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci COMMENT = '场景配置详情表' ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of tb_processdetail
-- ----------------------------

SET FOREIGN_KEY_CHECKS = 1;
