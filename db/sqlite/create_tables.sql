CREATE TABLE regions (
	region_id INTEGER PRIMARY KEY AUTOINCREMENT,
	region_name TEXT NOT NULL
);

CREATE TABLE countries (
	country_id TEXT NOT NULL,
	country_name TEXT NOT NULL,
	region_id INTEGER NOT NULL,
	PRIMARY KEY (country_id ASC),
	FOREIGN KEY (region_id) REFERENCES regions (region_id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE locations (
	location_id INTEGER PRIMARY KEY AUTOINCREMENT,
	street_address TEXT,
	postal_code TEXT,
	city TEXT NOT NULL,
	state_province TEXT,
	country_id TEXT NOT NULL,
	FOREIGN KEY (country_id) REFERENCES countries (country_id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE departments (
	department_id INTEGER PRIMARY KEY AUTOINCREMENT,
	department_name TEXT NOT NULL,
	location_id INTEGER NOT NULL,
	FOREIGN KEY (location_id) REFERENCES locations (location_id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE jobs (
	job_id INTEGER PRIMARY KEY AUTOINCREMENT,
	job_title TEXT NOT NULL,
	min_salary INTEGER NOT NULL,
	max_salary INTEGER NOT NULL
);

CREATE TABLE employees (
	employee_id INTEGER PRIMARY KEY AUTOINCREMENT,
	first_name TEXT,
	last_name TEXT NOT NULL,
	email TEXT NOT NULL,
	phone_number TEXT,
	hire_date TEXT NOT NULL,
	job_id INTEGER NOT NULL,
	salary INTEGER NOT NULL,
	manager_id INTEGER,
	department_id INTEGER NOT NULL,
	FOREIGN KEY (job_id) REFERENCES jobs (job_id) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (department_id) REFERENCES departments (department_id) ON DELETE CASCADE ON UPDATE CASCADE,
	FOREIGN KEY (manager_id) REFERENCES employees (employee_id) ON DELETE CASCADE ON UPDATE CASCADE
);

CREATE TABLE dependents (
	dependent_id INTEGER PRIMARY KEY AUTOINCREMENT,
	first_name TEXT NOT NULL,
	last_name TEXT NOT NULL,
	relationship TEXT NOT NULL,
	employee_id INTEGER NOT NULL,
	FOREIGN KEY (employee_id) REFERENCES employees (employee_id) ON DELETE CASCADE ON UPDATE CASCADE
);
