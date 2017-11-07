use agv;

#创建 station 表
create table agv_station (id INTEGER PRIMARY KEY AUTO_INCREMENT, station_x INTEGER, station_y INTEGER, station_type INTEGER, station_name text,station_lineAmount INTEGER,station_rfid INTEGER);

#创建 line 表
create table agv_line (id INTEGER PRIMARY KEY AUTO_INCREMENT,line_startX INTEGER,line_startY INTEGER,line_endX INTEGER,line_endY INTEGER,line_radius INTEGER,line_name text,line_clockwise BOOLEAN,line_line BOOLEAN,line_midX INTEGER,line_midY INTEGER,line_centerX INTEGER,line_centerY INTEGER,line_angle INTEGER,line_length INTEGER,line_startStation INTEGER,line_endStation INTEGER, line_draw BOOLEAN);

#创建 lmr 表
create table agv_lmr (id INTEGER PRIMARY KEY AUTO_INCREMENT,lmr_lastLine INTEGER,lmr_nextLine INTEGER,lmr_lmr INTEGER);

#创建 adj 表
create table agv_adj (id INTEGER PRIMARY KEY AUTO_INCREMENT,adj_startLine INTEGER,adj_endLine INTEGER);

#创建 log 表
create table agv_log (id INTEGER PRIMARY KEY AUTO_INCREMENT,log_level INTEGER,log_msg text,log_time datetime);

#创建 task 表
create table agv_task ( id INTEGER PRIMARY KEY AUTO_INCREMENT, task_producetime datetime,task_doTime datetime,task_doneTime datetime,task_excuteCar integer,task_status integer);

#创建 task_node 表
create table agv_task_node(id INTEGER PRIMARY KEY AUTO_INCREMENT, task_node_status integer,task_node_queuenumber integer,task_node_aimStation integer,task_node_waitType integer,task_node_waitTime integer,task_node_arriveTime datetime,task_node_leaveTime datetime,task_node_taskId integer);

#创建 user 表
CREATE TABLE agv_user(id INTEGER PRIMARY KEY AUTO_INCREMENT,user_username text,user_password text,user_realName TEXT,user_lastSignTime datetime,user_signState integer,user_sex bool,user_age int,user_createTime datetime,user_role INTEGER);

#创建 agv 表
CREATE TABLE agv_agv(id INTEGER PRIMARY KEY AUTO_INCREMENT,agv_name text,agv_ip text);

###############查询表是否存在
#select count(*) from INFORMATION_SCHEMA.TABLES where TABLE_NAME='agv_agv' ;

###############插入时返回自增长ID
#INSERT INTO agv_task (agv_name,agv_ip) VALUES ('123','192.168.1.5');SELECT @@Identity;