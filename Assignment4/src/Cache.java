
public class Cache {
	
	int numberOfBlocks;
	Block[] blockArray ;
	 Cache(int cacheSize,int blockSize)
	{
		numberOfBlocks = cacheSize/blockSize;
		blockArray = new Block[numberOfBlocks];
		for(int i=0;i<numberOfBlocks;i++)
		{
			blockArray[i] = new Block();
		}
	}
	
}
