import sqlite3
from datetime import datetime

# Path to database file
DATABASE_NAME = "/home/student/scripts/frostdetector.db"

def get_db():
    conn = sqlite3.connect(DATABASE_NAME)
    return conn

def get_by_id(id):
    db = get_db()
    cursor = db.cursor()
    statement = "SELECT * FROM sensors_data_v2 WHERE id = ?"
    cursor.execute(statement, [id])
    return cursor.fetchone()

def get_sensor_values():
    db = get_db()
    cursor = db.cursor()
    query = "SELECT * FROM sensors_data_v2"
    cursor.execute(query)
    return cursor.fetchall()

def insert_user(name, registration_token):
    db = get_db()
    cursor = db.cursor()
    statement = "INSERT INTO users(name, registration_token, created_at) VALUES (?, ?, ?)"
    cursor.execute(statement, [name, registration_token, datetime.now()])
    db.commit()
    return True

# Checks if the token is already registered in the database,
#  if not, it will be added
def verify_token(registration_token):
    db = get_db()
    cursor = db.cursor()
    cursor.execute("SELECT registration_token FROM users WHERE registration_token = ?", (registration_token,))
    data=cursor.fetchall()
    if len(data)==0:
        print('There is no user registered with registration token: ' + registration_token)
        print('Registering new user...')
        insert_user('Guest', registration_token)
    else:
        print('Registration token already registered')

def get_all_registration_tokens():
    db = get_db()
    cursor = db.cursor()
    statement = "SELECT registration_token FROM users"
    cursor.execute(statement)
    return cursor.fetchall()

