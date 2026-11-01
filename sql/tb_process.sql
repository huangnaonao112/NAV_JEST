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

 Date: 28/04/2026 16:28:36
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for tb_process
-- ----------------------------
DROP TABLE IF EXISTS `tb_process`;
CREATE TABLE `tb_process`  (
  `PROCESS_ID` int(11) NOT NULL AUTO_INCREMENT COMMENT '序号',
  `PROCESS_SCENE_NAME` varchar(128) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '场景名称',
  `PROCESS_SCENE_TYPE` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT NULL,
  `PROCESS_NAME` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NOT NULL COMMENT '测试类型',
  `scene_id` varchar(64) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT NULL COMMENT '场景唯一标识',
  `file_path` varchar(512) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT NULL COMMENT '场景文件路径',
  `access_device` varchar(256) CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL DEFAULT NULL COMMENT '接入设备',
  `description` text CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci NULL COMMENT '场景描述',
  `status` tinyint(4) NULL DEFAULT 1 COMMENT '状态：0禁用 1启用',
  `created_at` datetime NULL DEFAULT CURRENT_TIMESTAMP,
  `updated_at` datetime NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  PRIMARY KEY (`PROCESS_ID`) USING BTREE,
  UNIQUE INDEX `scene_id`(`scene_id`) USING BTREE,
  INDEX `idx_process_set`(`PROCESS_SCENE_NAME`) USING BTREE,
  INDEX `idx_process_name`(`PROCESS_NAME`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 31 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_unicode_ci ROW_FORMAT = DYNAMIC;

-- ----------------------------
-- Records of tb_process
-- ----------------------------

SET FOREIGN_KEY_CHECKS = 1;
