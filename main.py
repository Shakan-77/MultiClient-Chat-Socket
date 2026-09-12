import os
import json
import asyncio
import uuid
import boto3
from fastapi import FastAPI
from dotenv import load_dotenv

load_dotenv()

app = FastAPI()

# Initialize AWS Clients
session = boto3.Session(
    region_name=os.getenv("AWS_REGION"),
    aws_access_key_id=os.getenv("AWS_ACCESS_KEY_ID"),
    aws_secret_access_key=os.getenv("AWS_SECRET_ACCESS_KEY")
)

sqs = session.client('sqs')
dynamodb = session.resource('dynamodb')

QUEUE_URL = os.getenv("SQS_QUEUE_URL")
table = dynamodb.Table(os.getenv("DYNAMODB_TABLE"))

async def poll_sqs_queue():
    while True:
        try:
            response = await asyncio.to_thread(
                sqs.receive_message,
                QueueUrl=QUEUE_URL,
                MaxNumberOfMessages=10,
                WaitTimeSeconds=5
            )
            
            if 'Messages' in response:
                for message in response['Messages']:
                    body = json.loads(message['Body'])
                    
                    item = {
                        'messageId': str(uuid.uuid4()),
                        'userId': body.get('userId'),
                        'length': body.get('length')
                    }
                    
                    await asyncio.to_thread(table.put_item, Item=item)
                    
                    await asyncio.to_thread(
                        sqs.delete_message,
                        QueueUrl=QUEUE_URL,
                        ReceiptHandle=message['ReceiptHandle']
                    )
        except Exception as e:
            print(f"Polling error: {e}")
            
        await asyncio.sleep(1)

@app.on_event("startup")
async def startup_event():
    asyncio.create_task(poll_sqs_queue())

@app.get("/")
async def root():
    return {"status": "FastAPI SQS-to-DynamoDB Consumer is active."}