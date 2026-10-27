-- All monetary columns store Indian rupees directly, with two decimal places for paisa.
-- MySQL 8.0.16+ (CHECK constraints are enforced from this version).
-- This does not drop or overwrite an existing database.
CREATE DATABASE IF NOT EXISTS simple_bank;

USE simple_bank;

CREATE TABLE IF NOT EXISTS accounts (
    id INT AUTO_INCREMENT PRIMARY KEY,
    balance DECIMAL(14, 2) NOT NULL DEFAULT 0,
    active BOOLEAN NOT NULL DEFAULT TRUE,
    CHECK (
        balance >= 0
        AND balance <= 1000000000.00
    )
) ENGINE = InnoDB;

CREATE TABLE IF NOT EXISTS users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(32) UNIQUE NOT NULL,
    name VARCHAR(64) NOT NULL,
    password VARCHAR(64) NOT NULL,
    role ENUM(
        'customer',
        'employee',
        'manager',
        'admin'
    ) NOT NULL,
    active BOOLEAN NOT NULL DEFAULT TRUE,
    account_id INT NULL,
    CONSTRAINT uq_users_account UNIQUE (account_id),
    FOREIGN KEY (account_id) REFERENCES accounts (id)
) ENGINE = InnoDB;

CREATE TABLE IF NOT EXISTS transactions (
    id BIGINT AUTO_INCREMENT PRIMARY KEY,
    actor_id INT NOT NULL,
    from_account INT NULL,
    to_account INT NULL,
    kind VARCHAR(24) NOT NULL,
    amount DECIMAL(14, 2) NOT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (actor_id) REFERENCES users (id),
    FOREIGN KEY (from_account) REFERENCES accounts (id),
    FOREIGN KEY (to_account) REFERENCES accounts (id),
    CHECK (amount > 0)
) ENGINE = InnoDB;

CREATE TABLE IF NOT EXISTS loans (
    id INT AUTO_INCREMENT PRIMARY KEY,
    account_id INT NOT NULL,
    applicant_id INT NOT NULL,
    amount DECIMAL(14, 2) NOT NULL,
    remaining DECIMAL(14, 2) NOT NULL,
    status ENUM(
        'pending',
        'approved',
        'rejected',
        'repaid'
    ) NOT NULL DEFAULT 'pending',
    employee_id INT NULL,
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (account_id) REFERENCES accounts (id),
    FOREIGN KEY (applicant_id) REFERENCES users (id),
    FOREIGN KEY (employee_id) REFERENCES users (id),
    CHECK (
        amount > 0
        AND remaining >= 0
        AND remaining <= amount
    )
) ENGINE = InnoDB;

CREATE TABLE IF NOT EXISTS feedback (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    message VARCHAR(200) NOT NULL,
    FOREIGN KEY (user_id) REFERENCES users (id)
) ENGINE = InnoDB;
-- Create a dedicated application login separately. See README.md.