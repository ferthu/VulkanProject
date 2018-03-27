#include "Stuff/ObjReaderSimple.h"
#include "VulkanConstruct.h"

#include<map>



vk::QueryPool::QueryPool()
	: _pool(NULL), _cycleIndex(0), _size(0), _queryBuffer(NULL)
{}
vk::QueryPool::QueryPool(VkDevice dev, VkPhysicalDeviceProperties &props, VkQueryType queryType, uint32_t size, VkQueryPipelineStatisticFlags flags)
	: _pool(NULL), _cycleIndex(0), _size(size), _queryBuffer(NULL)
{
	// Fit params to query type
	switch (queryType)
	{
	case VkQueryType::VK_QUERY_TYPE_TIMESTAMP:
		if (!props.limits.timestampComputeAndGraphics)
			throw std::exception("Timestamps not supported on all queues...");
		_timeStampPeriod = props.limits.timestampPeriod;
		_stride = sizeof(uint64_t);
		break;
	default:
		throw std::exception("Only timestamp querypool supported...");
	}

	//Init metric buffer
	_bufSize = _stride * _size;
	_queryBuffer = new uint64_t[_bufSize / 8];
	_numQueries = 0;

	VkQueryPoolCreateInfo info = { };
	info.sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO;
	info.pNext = NULL;
	info.flags = 0;
	info.queryType = queryType;
	info.queryCount = size;
	info.pipelineStatistics = flags;

	VkResult err = vkCreateQueryPool(dev, &info, nullptr, &_pool);
	assert(err == VK_SUCCESS);
}
vk::QueryPool::~QueryPool()
{
}
void vk::QueryPool::destroy(VkDevice dev)
{
	vkDestroyQueryPool(dev, _pool, nullptr);
	if (_queryBuffer)
		delete _queryBuffer;
}

void vk::QueryPool::init(VkCommandBuffer cmdBuf)
{
	vkCmdResetQueryPool(cmdBuf, _pool, 0, _size);
}
vk::QueryFrame vk::QueryPool::newFrame(VkDevice dev)
{
	return vk::QueryFrame(*this, _cycleIndex);
}

/* Acquire a single query result from the currently acquired buffer. Returns time in seconds.
*/
double vk::QueryPool::getTimestampDiff(uint32_t queryPairIndex)
{
	return getTimestampDiff(queryPairIndex, queryPairIndex + 1);
}
/* Acquire a single query result from the currently acquired buffer. Returns time in seconds.
*/
double vk::QueryPool::getTimestampDiff(uint32_t beginQuery, uint32_t endQuery)
{
	const double toMS = 1.0 / std::pow(10, 6);
	uint64_t start = _queryBuffer[beginQuery], end = _queryBuffer[endQuery];
	std::cout << "Begin: " << start * toMS << ", End: " << end * toMS << ", Diff: " << (end - start) * toMS << "\n";
	return (end - start) * toMS / _timeStampPeriod;
}

vk::QueryFrame::QueryFrame()
	: _ref(NULL), _index(0), _count(0)
{
}
vk::QueryFrame::QueryFrame(vk::QueryPool &ref, uint32_t index)
	: _ref(&ref), _index(index), _count(0)
{
}

uint32_t vk::QueryFrame::next()
{
	uint32_t ind = _ref->_cycleIndex;
	_ref->_cycleIndex = (_ref->_cycleIndex + 1) % _ref->_size;
	_count++;
	assert(_count < _ref->_size);
	return ind;
}

void vk::QueryPool::resetBuf()
{
	for (uint32_t i = 0; i < _size; i++)
		_queryBuffer[i] = 0;
}

void vk::QueryFrame::beginQuery(VkCommandBuffer cmdBuf, VkQueryControlFlags flags)
{
	vkCmdBeginQuery(cmdBuf, _ref->_pool, next(), flags);
}
/* End an query for the frame
cmdBuf	<<	Related command buffer
queryID	<<	The index the query was launched within the frame
*/
void vk::QueryFrame::endQuery(VkCommandBuffer cmdBuf, uint32_t queryID)
{
	vkCmdEndQuery(cmdBuf, _ref->_pool, (_index + queryID) % _ref->_cycleIndex);
}


/* Perform a timestamp, query pool must be a timestamp pool.
*/
void vk::QueryFrame::timeStamp(VkCommandBuffer cmdBuf, VkPipelineStageFlagBits stage)
{
	uint32_t ind = next();
	vkCmdWriteTimestamp(cmdBuf, stage, _ref->_pool, ind);
}
/* Fetch query results
*/
VkResult vk::QueryFrame::fetchQuery(VkDevice dev, bool waitResult)
{
	if (!_ref) return VkResult::VK_NOT_READY;
	// Evaluate params (cyclic overlap & query call flags)
	uint32_t overlap = _index + _count;
	overlap = overlap > _ref->_size ? overlap - _ref->_size : 0;
	VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT;
	flags |= waitResult ? VK_QUERY_RESULT_WAIT_BIT : 0;
	_ref->_numQueries = _count;
	//Query
	_ref->resetBuf();
	VkResult err = vkGetQueryPoolResults(dev, _ref->_pool, _index, _count - overlap, _ref->_bufSize, (void*)_ref->_queryBuffer, _ref->_stride, flags);
	if (overlap > 0)
		VkResult err = vkGetQueryPoolResults(dev, _ref->_pool, 0, overlap, _ref->_bufSize - (_count - overlap) * 8, (void*)(_ref->_queryBuffer + (_count - overlap)), _ref->_stride, flags);
	return err;
}

void vk::QueryFrame::reset(VkCommandBuffer cmdBuf)
{
	if (!_ref) return;
	uint32_t overlap = _index + _count;
	overlap = overlap > _ref->_size ? overlap - _ref->_size : 0;
	vkCmdResetQueryPool(cmdBuf, _ref->_pool, _index, _count);
	if (overlap > 0)
		vkCmdResetQueryPool(cmdBuf, _ref->_pool, 0, overlap);
}

