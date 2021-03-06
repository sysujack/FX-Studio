#include "Material.h"
#include "Model.h"
#include "boost/algorithm/string.hpp"
#include <d3dcompiler.h>
#include <cctype>
#include <iostream>
#include <sstream>

Effect::Effect(ID3D11Device* pDevice, ID3DX11Effect* pD3DX11Effect)
	: m_Techniques(),
	m_TechniquesByName(),
	m_Variables(),
	m_VariablesByName()
{
	DEBUG_ASSERT(pDevice != nullptr);
	DEBUG_ASSERT(pD3DX11Effect != nullptr);

	D3DX11_EFFECT_DESC effectDesc;
	HRESULT hr = pD3DX11Effect->GetDesc(&effectDesc);
	if (FAILED(hr))
	{
		DEBUG_ERROR("ID3DX11Effect::GetDesc() failed.");
	}

	for (uint32_t i = 0; i < effectDesc.Techniques; i++)
	{
		Technique* technique = DEBUG_NEW Technique(pDevice, pD3DX11Effect->GetTechniqueByIndex(i));
		m_Techniques.push_back(technique);
		m_TechniquesByName.insert(std::make_pair(technique->GetTechniqueName(), technique));
	}

	for (uint32_t i = 0; i < effectDesc.GlobalVariables; i++)
	{
		Variable* variable = DEBUG_NEW Variable(pDevice, pD3DX11Effect->GetVariableByIndex(i));
		m_Variables.push_back(variable);
		m_VariablesByName.insert(std::make_pair(variable->GetVariableName(), variable));
	}
}

Effect::~Effect()
{
	for (auto technique : m_Techniques)
	{
		delete technique;
	}
	m_Techniques.clear();

	for (auto variable : m_Variables)
	{
		delete variable;
	}
	m_Variables.clear();
}

const std::vector<Technique*>& Effect::GetTechniques() const
{
	return m_Techniques;
}

const std::map<std::string, Technique*>& Effect::GetTechniquesByName() const
{
	return m_TechniquesByName;
}

const std::vector<Variable*>& Effect::GetVariables() const
{
	return m_Variables;
}

const std::map<std::string, Variable*>& Effect::GetVariablesByName() const
{
	return m_VariablesByName;
}

uint32_t Effect::GenerateXml(const std::string& effectObjectPath, const std::string& effectName, bool reLoad)
{
	if (m_effectXmlString.empty() || reLoad)
	{
		unique_ptr<tinyxml2::XMLDocument> pEffectXmlDoc = std::unique_ptr<tinyxml2::XMLDocument>(DEBUG_NEW tinyxml2::XMLDocument());

		tinyxml2::XMLElement* pRoot = pEffectXmlDoc->NewElement("Material");
		pEffectXmlDoc->InsertEndChild(pRoot);

		pRoot->SetAttribute("effect", effectName.c_str());
		pRoot->SetAttribute("object", effectObjectPath.c_str());

		tinyxml2::XMLElement* pTechniques = pEffectXmlDoc->NewElement("Techniques");
		pRoot->InsertEndChild(pTechniques);

		bool firstCheck = true;
		for (auto technique : m_Techniques)
		{
			tinyxml2::XMLElement* pChildTechnique = pEffectXmlDoc->NewElement("Technique");
			pChildTechnique->SetAttribute("name", technique->GetTechniqueName().c_str());
			if (firstCheck)
			{
				pChildTechnique->SetAttribute("checked", true);
				firstCheck = false;
			}
			else
				pChildTechnique->SetAttribute("checked", false);

			for (auto pass : technique->GetPasses())
			{
				tinyxml2::XMLElement* pChildPass = pEffectXmlDoc->NewElement("Pass");
				pChildPass->InsertEndChild(pEffectXmlDoc->NewText(pass->GetPassName().c_str()));
				pChildTechnique->InsertEndChild(pChildPass);
			}

			pTechniques->InsertEndChild(pChildTechnique);
		}

		tinyxml2::XMLElement* pVariables = pEffectXmlDoc->NewElement("Variables");
		pRoot->InsertEndChild(pVariables);

		for (auto variable : m_Variables)
		{
			tinyxml2::XMLElement* pChildVariable = pEffectXmlDoc->NewElement(variable->GetVariableName().c_str());
			for (auto annotation : variable->GetAnnotations())
			{
				pChildVariable->SetAttribute(annotation->GetAnnotationName().c_str(), annotation->GetAnnotationValue().c_str());
			}
			pChildVariable->SetText(variable->GetVariableValue().c_str());

			pVariables->InsertEndChild(pChildVariable);
		}

		tinyxml2::XMLPrinter printer;
		pEffectXmlDoc->Accept(&printer);
		m_effectXmlString = std::string(printer.CStr(), printer.CStrSize());
	}
	
	return (uint32_t)m_effectXmlString.length();
}

Technique::Technique(ID3D11Device* pDevice, ID3DX11EffectTechnique* pD3DX11EffectTechnique)
	: m_pD3DX11EffectTechnique(pD3DX11EffectTechnique),
	m_D3DX11TechniqueDesc(),
	m_TechniqueName(),
	m_Passes(),
	m_PassesByName()
{
	m_pD3DX11EffectTechnique->GetDesc(&m_D3DX11TechniqueDesc);
	m_TechniqueName = m_D3DX11TechniqueDesc.Name;
	for (uint32_t i = 0; i < m_D3DX11TechniqueDesc.Passes; i++)
	{
		Pass* pass = DEBUG_NEW Pass(pDevice, m_pD3DX11EffectTechnique->GetPassByIndex(i));
		m_Passes.push_back(pass);
		m_PassesByName.insert(std::make_pair(pass->GetPassName(), pass));
	}
}

Technique::~Technique()
{
	for (auto pass : m_Passes)
	{
		SAFE_DELETE(pass);
	}
	m_Passes.clear();
}

const std::string& Technique::GetTechniqueName() const
{
	return m_TechniqueName;
}

const std::vector<Pass*>& Technique::GetPasses() const
{
	return m_Passes;
}

const std::map<std::string, Pass*>& Technique::GetPassesByName() const
{
	return m_PassesByName;
}

Pass::Pass(ID3D11Device* pDevice, ID3DX11EffectPass* pD3DX11EffectPass)
	: p_Device(pDevice),
	m_pD3DX11EffectPass(pD3DX11EffectPass),
	m_PassName(),
	m_pInputLayouts(nullptr),
	m_VertexFormat(),
	m_VertexSize(0),
	m_HasGeometryShader(false),
	m_HasHullShader(false),
	m_HasDomainShader(false),
	m_TessPrimitive(0)
{
	D3DX11_PASS_DESC passDesc;
	m_pD3DX11EffectPass->GetDesc(&passDesc);
	m_PassName = passDesc.Name;

	D3DX11_PASS_SHADER_DESC vertexShaderDesc;
	m_pD3DX11EffectPass->GetVertexShaderDesc(&vertexShaderDesc);
	ID3DX11EffectShaderVariable* pShaderVariable = vertexShaderDesc.pShaderVariable;
	if (pShaderVariable != nullptr && pShaderVariable->IsValid())
	{
		D3DX11_EFFECT_SHADER_DESC shaderDesc;
		pShaderVariable->GetShaderDesc(vertexShaderDesc.ShaderIndex, &shaderDesc);
		std::vector<D3D11_INPUT_ELEMENT_DESC> inputElementDescs(shaderDesc.NumInputSignatureEntries);

		for (uint32_t i = 0; i < shaderDesc.NumInputSignatureEntries; i++)
		{
			D3D11_SIGNATURE_PARAMETER_DESC parameterDesc;
			vertexShaderDesc.pShaderVariable->GetInputSignatureElementDesc(vertexShaderDesc.ShaderIndex, i, &parameterDesc);
			inputElementDescs[i].SemanticName = parameterDesc.SemanticName;
			inputElementDescs[i].SemanticIndex = parameterDesc.SemanticIndex;
			inputElementDescs[i].Format = GetElementFormat(parameterDesc.ComponentType, parameterDesc.Mask);
			inputElementDescs[i].InputSlot = 0;
			inputElementDescs[i].AlignedByteOffset = parameterDesc.Register > 0 ? D3D11_APPEND_ALIGNED_ELEMENT : 0;
			inputElementDescs[i].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
			inputElementDescs[i].InstanceDataStepRate = 0;

			m_VertexFormat.push_back(std::pair<std::string, int>(inputElementDescs[i].SemanticName, (parameterDesc.Mask + 1) / 4));
		}
	
		pDevice->CreateInputLayout(
			&inputElementDescs[0], inputElementDescs.size(), passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &m_pInputLayouts);
	}

	D3DX11_PASS_SHADER_DESC geometryShaderDesc;
	m_pD3DX11EffectPass->GetGeometryShaderDesc(&geometryShaderDesc);
	pShaderVariable = geometryShaderDesc.pShaderVariable;
	if (pShaderVariable != nullptr && pShaderVariable->IsValid())
	{
		m_HasGeometryShader = true;
	}
	else
	{
		m_HasGeometryShader = false;
	}

	D3DX11_PASS_SHADER_DESC hullShaderDesc;
	m_pD3DX11EffectPass->GetHullShaderDesc(&hullShaderDesc);
	pShaderVariable = hullShaderDesc.pShaderVariable;
	if (pShaderVariable != nullptr && pShaderVariable->IsValid())
	{
		D3DX11_EFFECT_SHADER_DESC shaderDesc;
		pShaderVariable->GetShaderDesc(hullShaderDesc.ShaderIndex, &shaderDesc);
		D3D11_SIGNATURE_PARAMETER_DESC parameterDesc;
		for (uint32_t i = 0; i < shaderDesc.NumPatchConstantSignatureEntries; i++)
		{
			pShaderVariable->GetPatchConstantSignatureElementDesc(hullShaderDesc.ShaderIndex, i, &parameterDesc);
			if (boost::iequals(parameterDesc.SemanticName, "SV_TessFactor"))
			{
				m_TessPrimitive++;
			}
		}
		m_HasHullShader = true;
	}
	else
	{
		m_HasHullShader = false;
	}

	D3DX11_PASS_SHADER_DESC domainShaderDesc;
	m_pD3DX11EffectPass->GetDomainShaderDesc(&domainShaderDesc);
	pShaderVariable = domainShaderDesc.pShaderVariable;
	if (pShaderVariable != nullptr && pShaderVariable->IsValid())
	{
		m_HasDomainShader = true;
	}
	else
	{
		m_HasDomainShader = false;
	}
}

void Pass::CreateVertexBuffer(const void* pVertexData, uint32_t size, ID3D11Buffer** ppVertexBuffer) const
{
	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.ByteWidth = size;
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexSubResourceData;
	ZeroMemory(&vertexSubResourceData, sizeof(vertexSubResourceData));
	vertexSubResourceData.pSysMem = pVertexData;

	if (FAILED(p_Device->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, ppVertexBuffer)))
	{
		DEBUG_ERROR("ID3D11Device::CreateBuffer() failed.");
	}
}

void Pass::CreateIndexBuffer(const void* pIndexData, uint32_t size, ID3D11Buffer** ppIndexBuffer) const
{
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
	indexBufferDesc.ByteWidth = size;
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexSubResourceData;
	ZeroMemory(&indexSubResourceData, sizeof(indexSubResourceData));
	indexSubResourceData.pSysMem = pIndexData;

	if (FAILED(p_Device->CreateBuffer(&indexBufferDesc, &indexSubResourceData, ppIndexBuffer)))
	{
		DEBUG_ERROR("ID3D11Device::CreateBuffer() failed.");
	}
}

void Pass::CreateVertexBuffer(const Mesh* mesh, ID3D11Buffer** ppVertexBuffer) const
{
	const std::vector<Vector3>& vertices = mesh->GetVertices();
	uint32_t vertexCount = vertices.size();

	const std::vector<std::vector<Vector2> >& textureCoordinates = mesh->GetTextureCoordinates();
	const std::vector<std::vector<Vector4> >& vertexColors = mesh->GetVertexColors();
	const std::vector<Vector3>& normals = mesh->GetNormals();
	const std::vector<Vector3>& tangents = mesh->GetTangents();
	const std::vector<Vector3>& binormals = mesh->GetBiNormals();

	std::vector<float> vertexData;
	vertexData.reserve(vertexCount * m_VertexSize);

	for (uint32_t i = 0; i < vertexCount; i++)
	{
		for (auto& vertexFormat : m_VertexFormat)
		{
			if (vertexFormat.first == "POSITION")
			{
				vertexData.push_back(vertices.at(i).x);
				vertexData.push_back(vertices.at(i).y);
				vertexData.push_back(vertices.at(i).z);
				if (vertexFormat.second > 3)
				{
					vertexData.push_back(1.0f);
				}
			}
			else if (vertexFormat.first == "COLOR")
			{
				if (!vertexColors.empty() && !vertexColors[0].empty())
				{
					vertexData.push_back(vertexColors[0].at(i).x);
					vertexData.push_back(vertexColors[0].at(i).y);
					vertexData.push_back(vertexColors[0].at(i).z);
					vertexData.push_back(vertexColors[0].at(i).w);
				}
				else
				{
					vertexData.push_back(1.0f);
					vertexData.push_back(1.0f);
					vertexData.push_back(1.0f);
					vertexData.push_back(1.0f);
				}
			}
			else if (vertexFormat.first == "TEXCOORD")
			{
				if (!textureCoordinates.empty() && !textureCoordinates[0].empty())
				{
					vertexData.push_back(textureCoordinates[0].at(i).x);
					vertexData.push_back(textureCoordinates[0].at(i).y);
				}
				else
				{
					vertexData.push_back(0.5f);
					vertexData.push_back(0.5f);
				}
				if (vertexFormat.second > 3)
				{
					vertexData.push_back(0.0f);
					vertexData.push_back(0.0f);
				}
			}
			else if (vertexFormat.first == "NORMAL")
			{
				if (!normals.empty())
				{
					vertexData.push_back(normals.at(i).x);
					vertexData.push_back(normals.at(i).y);
					vertexData.push_back(normals.at(i).z);
				}
				else
				{
					vertexData.push_back(0.0f);
					vertexData.push_back(0.0f);
					vertexData.push_back(0.0f);
				}
				if (vertexFormat.second > 3)
				{
					vertexData.push_back(0.0f);
				}
			}
			else if (vertexFormat.first == "TANGENT")
			{
				if (!tangents.empty())
				{
					vertexData.push_back(tangents.at(i).x);
					vertexData.push_back(tangents.at(i).y);
					vertexData.push_back(tangents.at(i).z);
				}
				else
				{
					vertexData.push_back(0.0f);
					vertexData.push_back(0.0f);
					vertexData.push_back(0.0f);
				}
				if (vertexFormat.second > 3)
				{
					vertexData.push_back(0.0f);
				}
			}
			else if (vertexFormat.first == "BINORMAL")
			{
				if (!binormals.empty())
				{
					vertexData.push_back(binormals.at(i).x);
					vertexData.push_back(binormals.at(i).y);
					vertexData.push_back(binormals.at(i).z);
				}
				else
				{
					vertexData.push_back(0.0f);
					vertexData.push_back(0.0f);
					vertexData.push_back(0.0f);
				}
				if (vertexFormat.second > 3)
				{
					vertexData.push_back(0.0f);
				}
			}
		}
	}

	D3D11_BUFFER_DESC vertexBufferDesc;
	ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
	vertexBufferDesc.ByteWidth = m_VertexSize * vertexCount;
	vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexSubResourceData;
	ZeroMemory(&vertexSubResourceData, sizeof(vertexSubResourceData));
	vertexSubResourceData.pSysMem = &vertexData.front();

	if (FAILED(p_Device->CreateBuffer(&vertexBufferDesc, &vertexSubResourceData, ppVertexBuffer)))
	{
		DEBUG_ERROR("ID3D11Device::CreateBuffer() failed.");
	}
}

void Pass::CreateIndexBuffer(const Mesh* mesh, ID3D11Buffer** ppIndexBuffer) const
{
	D3D11_BUFFER_DESC indexBufferDesc;
	ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
	indexBufferDesc.ByteWidth = sizeof(uint32_t) * (uint32_t)mesh->GetIndices().size();
	indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexSubResourceData;
	ZeroMemory(&indexSubResourceData, sizeof(indexSubResourceData));
	indexSubResourceData.pSysMem = &(mesh->GetIndices().front());

	if (FAILED(p_Device->CreateBuffer(&indexBufferDesc, &indexSubResourceData, ppIndexBuffer)))
	{
		DEBUG_ERROR("ID3D11Device::CreateBuffer() failed.");
	}
}

void Pass::Apply(uint32_t flags, ID3D11DeviceContext* pDeviceContext)
{
	m_pD3DX11EffectPass->Apply(flags, pDeviceContext);
}

DXGI_FORMAT Pass::GetElementFormat(D3D_REGISTER_COMPONENT_TYPE compoentType, uint8_t mask)
{
	switch (compoentType)
	{
	case D3D_REGISTER_COMPONENT_UNKNOWN:
		return DXGI_FORMAT_UNKNOWN;
	case D3D_REGISTER_COMPONENT_UINT32:
		switch (mask & 0xf)
		{
		case 15: m_VertexSize += 16; return DXGI_FORMAT_R32G32B32A32_UINT;
		case 7: m_VertexSize += 12; return DXGI_FORMAT_R32G32B32_UINT;
		case 3: m_VertexSize += 8; return DXGI_FORMAT_R32G32_UINT;
		case 1: m_VertexSize += 4; return DXGI_FORMAT_R32_UINT;
		default: return DXGI_FORMAT_UNKNOWN;
		}
	case D3D_REGISTER_COMPONENT_SINT32:
		DEBUG_ERROR("need to add new format!");
		return DXGI_FORMAT_UNKNOWN;
	case D3D_REGISTER_COMPONENT_FLOAT32:
	{
		switch (mask & 0xf)
		{
		case 15: m_VertexSize += 16; return DXGI_FORMAT_R32G32B32A32_FLOAT;
		case 7: m_VertexSize += 12; return DXGI_FORMAT_R32G32B32_FLOAT;
		case 3: m_VertexSize += 8; return DXGI_FORMAT_R32G32_FLOAT;
		case 1: m_VertexSize += 4; return DXGI_FORMAT_R32_FLOAT;
		default: return DXGI_FORMAT_UNKNOWN;
		}
	}
	default:
		return DXGI_FORMAT_UNKNOWN;
	}
}

Variable::Variable(ID3D11Device* pDevice, ID3DX11EffectVariable* pD3DX11EffectVariable)
	: m_pD3DX11EffectVariable(pD3DX11EffectVariable),
	m_pD3DX11EffectType(nullptr),
	m_ElementsCount(0)
{
	D3DX11_EFFECT_VARIABLE_DESC variableDesc;
	m_pD3DX11EffectVariable->GetDesc(&variableDesc);
	m_VariableName = variableDesc.Name;
	if (variableDesc.Semantic != nullptr)
	{
		m_VariableSemantic = variableDesc.Semantic;
		std::transform(m_VariableSemantic.begin(), m_VariableSemantic.end(), m_VariableSemantic.begin(), (int(*)(int)) std::tolower);
	}

	m_pD3DX11EffectType = m_pD3DX11EffectVariable->GetType();
	D3DX11_EFFECT_TYPE_DESC typeDesc;
	m_pD3DX11EffectType->GetDesc(&typeDesc);
	m_VariableTypeName = typeDesc.TypeName; 
	m_pD3DX11EffectType->GetDesc(&typeDesc);

	switch (typeDesc.Class)
	{
	case D3D_SHADER_VARIABLE_CLASS::D3D_SVC_SCALAR:
	{
		ID3DX11EffectScalarVariable* scalarVal = m_pD3DX11EffectVariable->AsScalar();
		std::stringstream ss;

		switch (typeDesc.Type)
		{
		case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_INT:
		{
			switch (typeDesc.Elements)
			{
			case 0:
			{
				int value;
				scalarVal->GetInt(&value);
				ss << value;
				break;
			}
			case 2:
			{
				int value[2];
				scalarVal->GetIntArray(value, 0, typeDesc.Elements);
				ss << value[0] << " " << value[1];
				break;
			}
			case 3:
			{
				int value[3];
				scalarVal->GetIntArray(value, 0, typeDesc.Elements);
				ss << value[0] << " " << value[1] << " " << value[2];
				break;
			}
			case 4:
			{
				int value[4];
				scalarVal->GetIntArray(value, 0, typeDesc.Elements);
				ss << value[0] << " " << value[1] << " " << value[2] << " " << value[3];
				break;
			}
			default:
				break;
			}
			break;
		}
		case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_FLOAT:
		{
			switch (typeDesc.Elements)
			{
			case 0:
			{
				float value;
				scalarVal->GetFloat(&value);
				ss << value;
				break;
			}
			case 2:
			{
				Vector2 value;
				scalarVal->GetFloatArray(reinterpret_cast<float*>(&value), 0, typeDesc.Elements);
				ss << value.x << " " << value.y;
				break;
			}
			case 3:
			{
				Vector3 value;
				scalarVal->GetFloatArray(reinterpret_cast<float*>(&value), 0, typeDesc.Elements);
				ss << value.x << " " << value.y << " " << value.z;
				break;
			}
			case 4:
			{
				Vector4 value;
				scalarVal->GetFloatArray(reinterpret_cast<float*>(&value), 0, typeDesc.Elements);
				ss << value.x << " " << value.y << " " << value.z << " " << value.w;
				break;
			}
			default:
				break;
			}
			break;
		}
		default:
			break;
		}

		m_VariableValue = ss.str();
		break;
	}
	case D3D_SHADER_VARIABLE_CLASS::D3D_SVC_VECTOR:
	{
		ID3DX11EffectVectorVariable* vectroVal = m_pD3DX11EffectVariable->AsVector();
		std::stringstream ss;

		switch (typeDesc.Columns)
		{
		case 2:
		{
			Vector2 value;
			vectroVal->GetFloatVector(reinterpret_cast<float*>(&value));
			ss << value.x << " " << value.y;
			break;
		}
		case 3:
		{
			Vector3 value;
			vectroVal->GetFloatVector(reinterpret_cast<float*>(&value));
			ss << value.x << " " << value.y << " " << value.z;
			break;
		}
		case 4:
		{
			Vector4 value;
			vectroVal->GetFloatVector(reinterpret_cast<float*>(&value));
			ss << value.x << " " << value.y << " " << value.z << " " << value.w;
			break;
		}
		default:
			break;
		}

		m_VariableValue = ss.str();
		break;
	}
	case D3D_SHADER_VARIABLE_CLASS::D3D_SVC_MATRIX_COLUMNS:
	case D3D_SHADER_VARIABLE_CLASS::D3D_SVC_MATRIX_ROWS:
	{
		ID3DX11EffectMatrixVariable* matrixVal = m_pD3DX11EffectVariable->AsMatrix();
		Matrix value;
		matrixVal->GetMatrix(reinterpret_cast<float*>(&value));

		std::stringstream ss;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				ss << value.m[i][j] << " ";
			}
		}
		m_VariableValue = ss.str();
		break;
	}
	case D3D_SHADER_VARIABLE_CLASS::D3D_SVC_OBJECT:
	{
		switch (typeDesc.Type)
		{
		case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_RASTERIZER:
		{
			ID3DX11EffectRasterizerVariable* rasterizerVal = m_pD3DX11EffectVariable->AsRasterizer();
			break;
		}
		case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_BLEND:
		{
			ID3DX11EffectBlendVariable* blendVal = m_pD3DX11EffectVariable->AsBlend();
			break;
		}
		case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_DEPTHSTENCIL:
		{
			ID3DX11EffectDepthStencilVariable* depthVal = m_pD3DX11EffectVariable->AsDepthStencil();
			break;
		}
		default:
			break;
		}

		break;
	}
	default:
		break;
	}

	for (uint32_t i = 0; i < variableDesc.Annotations; i++)
	{
		Annotation* annotation = DEBUG_NEW Annotation(m_pD3DX11EffectVariable->GetAnnotationByIndex(i));
		m_Annotations.push_back(annotation);
	}
}

void Variable::SetMatrix(CXMMATRIX value)
{
	ID3DX11EffectMatrixVariable* variable = m_pD3DX11EffectVariable->AsMatrix();
	if (!variable->IsValid())
	{
		DEBUG_ERROR("Invalid effect variable cast.");
	}

	variable->SetMatrix(reinterpret_cast<const float*>(&value));
}

void Variable::SetResource(ID3D11ShaderResourceView* value)
{
	ID3DX11EffectShaderResourceVariable* variable = m_pD3DX11EffectVariable->AsShaderResource();
	if (!variable->IsValid())
	{
		DEBUG_ERROR("Invalid effect variable cast.");
	}

	variable->SetResource(value);
}

void Variable::SetVector(FXMVECTOR value)
{
	ID3DX11EffectVectorVariable* variable = m_pD3DX11EffectVariable->AsVector();
	if (!variable->IsValid())
	{
		DEBUG_ERROR("Invalid effect variable cast.");
	}

	variable->SetFloatVector(reinterpret_cast<const float*>(&value));
}

void Variable::SetFloat(float value)
{
	ID3DX11EffectScalarVariable* variable = m_pD3DX11EffectVariable->AsScalar();
	if (!variable->IsValid())
	{
		DEBUG_ERROR("Invalid effect variable cast.");
	}

	variable->SetFloat(value);
}

void Variable::SetFloatArray(const std::vector<float>& values)
{
	ID3DX11EffectScalarVariable* variable = m_pD3DX11EffectVariable->AsScalar();
	if (!variable->IsValid())
	{
		DEBUG_ERROR("Invalid effect variable cast.");
	}

	variable->SetFloatArray(&values[0], 0, (uint32_t)values.size());
}

void Variable::SetInt(int value)
{
	ID3DX11EffectScalarVariable* variable = m_pD3DX11EffectVariable->AsScalar();
	if (!variable->IsValid())
	{
		DEBUG_ERROR("Invalid effect variable cast.");
	}

	variable->SetInt(value);
}

Variable& Variable::operator<<(ID3D11UnorderedAccessView* value)
{
	ID3DX11EffectUnorderedAccessViewVariable* variable = m_pD3DX11EffectVariable->AsUnorderedAccessView();
	if (!variable->IsValid())
	{
		DEBUG_ERROR("Invalid effect variable cast.");
	}

	variable->SetUnorderedAccessView(value);

	return *this;
}

Variable& Variable::operator<<(const std::vector<XMFLOAT2>& values)
{
	ID3DX11EffectVectorVariable* variable = m_pD3DX11EffectVariable->AsVector();
	if (!variable->IsValid())
	{
		DEBUG_ERROR("Invalid effect variable cast.");
	}

	variable->SetFloatVectorArray(reinterpret_cast<const float*>(&values[0]), 0, (uint32_t)values.size());

	return *this;
}

Variable& Variable::operator<<(const std::vector<XMFLOAT4X4>& values)
{
	ID3DX11EffectMatrixVariable* variable = m_pD3DX11EffectVariable->AsMatrix();
	if (!variable->IsValid())
	{
		DEBUG_ERROR("Invalid effect variable cast.");
	}

	variable->SetMatrixArray(reinterpret_cast<const float*>(&values[0]), 0, (uint32_t)values.size());

	return *this;
}

Annotation::Annotation(ID3DX11EffectVariable* pAnnotation)
{
	ID3DX11EffectType* annotationType = pAnnotation->GetType();
	D3DX11_EFFECT_TYPE_DESC typeDesc;
	annotationType->GetDesc(&typeDesc);

	switch (typeDesc.Type)
	{
	case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_STRING:
	{
		ID3DX11EffectStringVariable* stringVal = pAnnotation->AsString();
		LPCSTR pString = nullptr;
		stringVal->GetString(&pString);
		m_AnnotationValue = pString;
		break;
	}
	case D3D_SHADER_VARIABLE_TYPE::D3D_SVT_FLOAT:
	{
		ID3DX11EffectScalarVariable* scalarVal = pAnnotation->AsScalar();
		float value;
		scalarVal->GetFloat(&value);
		m_AnnotationValue = std::to_string(value);
		break;
	}
	default:
		break;
	}

	D3DX11_EFFECT_VARIABLE_DESC annotationDesc;
	pAnnotation->GetDesc(&annotationDesc);
	m_AnnotationName = annotationDesc.Name;
	std::transform(m_AnnotationName.begin(), m_AnnotationName.end(), m_AnnotationName.begin(), (int(*)(int)) std::tolower);
}
